#include "fundamental/file/file.h"
#include "fileAdaptive.h"
#include "fundamental/memory/memory.h"
#include "fundamental/error/error.h"

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <stddef.h>

typedef unsigned long size_t;
typedef long off_t;

#define SYS_open 2
#define SYS_close 3
#define SYS_fstat 5
#define SYS_mmap 9
#define SYS_munmap 11
#define SYS_ftruncate 77
#define SYS_io_uring_setup 425
#define SYS_io_uring_enter 426

#define O_RDWR 2
#define O_WRONLY 1
#define O_CREAT 0100
#define O_APPEND 02000

#define PROT_READ 0x1
#define PROT_WRITE 0x2
#define MAP_SHARED 0x1

#define IORING_OP_WRITEV 2
#define IORING_ENTER_GETEVENTS 0x01

#define PAGE_SIZE 4096

struct stat {
	unsigned long st_dev;
	unsigned long st_ino;
	unsigned long st_nlink;
	unsigned int st_mode;
	unsigned int st_uid;
	unsigned int st_gid;
	unsigned long st_rdev;
	unsigned long st_size;
	unsigned long st_blksize;
	unsigned long st_blocks;
	unsigned long st_atime;
	unsigned long st_atime_nsec;
	unsigned long st_mtime;
	unsigned long st_mtime_nsec;
	unsigned long st_ctime;
	unsigned long st_ctime_nsec;
	unsigned long __unused[3];
};

struct io_uring_sqe {
	uint8_t opcode;
	uint8_t flags;
	uint16_t ioprio;
	int32_t fd;
	uint64_t off;
	uint64_t addr;
	uint32_t len;
	int rw_flags;
	uint64_t user_data;
	uint16_t buf_index;
	uint16_t __pad2[3];
};

struct io_uring_cqe {
	uint64_t user_data;
	int32_t res;
	uint32_t flags;
};

struct io_uring_params {
	uint32_t sq_entries;
	uint32_t cq_entries;
	uint32_t flags;
	uint32_t sq_thread_cpu;
	uint32_t sq_thread_idle;
	uint32_t features;
	uint32_t wq_fd;
	uint32_t resv[3];
};

/* MMap append: open → fstat+ftruncate → mmap → memcpy */
typedef struct {
	Append parameters;
	int file_descriptor;
	uint64_t aligned_offset; /* page-aligned offset for mmap */
	uint64_t view_size; /* bytes_to_append + intra_page_offset */
	void *mapped_address;
	bool file_opened;
	bool file_extended;
	bool mapped;
} MMapAppendState;

/* Ring append: io_uring_setup → open(O_APPEND) → writev(off=-1) → poll cqe */
typedef struct {
	Append parameters;
	int ring_fd;
	int file_fd;
	void *sq_ring;
	void *cq_ring;
	bool ring_initialized;
	bool file_opened;
	bool io_submitted;
} RingAppendState;

static inline long syscall1(long n, long a1)
{
	long ret;
	__asm__ __volatile__("syscall"
						 : "=a"(ret)
						 : "a"(n), "D"(a1)
						 : "rcx", "r11", "memory");
	return ret;
}

static inline long syscall2(long n, long a1, long a2)
{
	long ret;
	__asm__ __volatile__("syscall"
						 : "=a"(ret)
						 : "a"(n), "D"(a1), "S"(a2)
						 : "rcx", "r11", "memory");
	return ret;
}

static inline long syscall3(long n, long a1, long a2, long a3)
{
	long ret;
	__asm__ __volatile__("syscall"
						 : "=a"(ret)
						 : "a"(n), "D"(a1), "S"(a2), "d"(a3)
						 : "rcx", "r11", "memory");
	return ret;
}

static inline long syscall4(long n, long a1, long a2, long a3, long a4)
{
	long ret;
	register long r10 __asm__("r10") = a4;
	__asm__ __volatile__("syscall"
						 : "=a"(ret)
						 : "a"(n), "D"(a1), "S"(a2), "d"(a3), "r"(r10)
						 : "rcx", "r11", "memory");
	return ret;
}

static inline long syscall6(long n, long a1, long a2, long a3, long a4, long a5,
							long a6)
{
	long ret;
	register long r10 __asm__("r10") = a4;
	register long r8 __asm__("r8") = a5;
	register long r9 __asm__("r9") = a6;
	__asm__ __volatile__("syscall"
						 : "=a"(ret)
						 : "a"(n), "D"(a1), "S"(a2), "d"(a3), "r"(r10), "r"(r8),
						   "r"(r9)
						 : "rcx", "r11", "memory");
	return ret;
}

static AsyncStatus poll_mmap_append(AsyncResult *result)
{
	MMapAppendState *state = (MMapAppendState *)result->state;
	FileAdaptiveState *adaptive = state->parameters.adaptive;
	uint64_t bytes = state->parameters.bytes_to_append;
	AsyncStatus final_status = ASYNC_COMPLETED;

	if (!state->file_opened) {
		int fd = (int)syscall3(SYS_open, (long)state->parameters.file_path,
							   O_RDWR | O_CREAT, 0644);
		if (fd < 0) {
			result->error =
				fun_error_result(-fd, "Failed to open file for append");
			return ASYNC_ERROR;
		}
		state->file_descriptor = fd;
		state->file_opened = true;
		return ASYNC_PENDING;
	}

	if (!state->file_extended) {
		struct stat file_stat;
		if (syscall2(SYS_fstat, state->file_descriptor, (long)&file_stat) < 0) {
			result->error = fun_error_result(1, "Failed to stat file");
			final_status = ASYNC_ERROR;
			goto cleanup;
		}
		uint64_t append_offset = file_stat.st_size;
		uint64_t new_size = append_offset + state->parameters.bytes_to_append;
		if (syscall2(SYS_ftruncate, state->file_descriptor, (long)new_size) <
			0) {
			result->error = fun_error_result(1, "Failed to extend file");
			final_status = ASYNC_ERROR;
			goto cleanup;
		}
		/* store alignment info for the mmap step */
		state->aligned_offset = (append_offset / PAGE_SIZE) * PAGE_SIZE;
		state->view_size = state->parameters.bytes_to_append +
						   (append_offset - state->aligned_offset);
		state->file_extended = true;
		return ASYNC_PENDING;
	}

	if (!state->mapped) {
		void *mapped = (void *)syscall6(SYS_mmap, 0, (long)state->view_size,
										PROT_READ | PROT_WRITE, MAP_SHARED,
										state->file_descriptor,
										(long)state->aligned_offset);
		if ((long)mapped < 0) {
			result->error =
				fun_error_result(1, "Failed to mmap file for append");
			final_status = ASYNC_ERROR;
			goto cleanup;
		}
		state->mapped_address = mapped;
		state->mapped = true;
		return ASYNC_PENDING;
	}

	/* write data at intra-page offset within the mapping */
	uint64_t intra = state->view_size - state->parameters.bytes_to_append;
	void *write_ptr = (char *)state->mapped_address + intra;
	fun_memory_copy(state->parameters.input, write_ptr,
					state->parameters.bytes_to_append);

cleanup:
	if (state->mapped_address && state->mapped_address != (void *)-1)
		syscall2(SYS_munmap, (long)state->mapped_address,
				 (long)state->view_size);
	if (state->file_descriptor >= 0)
		syscall1(SYS_close, state->file_descriptor);
	fun_memory_free((Memory *)&state);
	if (final_status == ASYNC_COMPLETED)
		file_adaptive_update(adaptive, bytes);
	return final_status;
}

static AsyncResult create_mmap_append(Append parameters)
{
	MemoryResult mem_result = fun_memory_allocate(sizeof(MMapAppendState));
	if (fun_error_is_error(mem_result.error))
		return (AsyncResult){ .status = ASYNC_ERROR,
							  .error = mem_result.error };

	MMapAppendState *state = (MMapAppendState *)mem_result.value;
	*state = (MMapAppendState){ .parameters = parameters,
								.file_descriptor = -1,
								.aligned_offset = 0,
								.view_size = 0,
								.mapped_address = NULL,
								.file_opened = false,
								.file_extended = false,
								.mapped = false };

	return (AsyncResult){ .state = state,
						  .poll = poll_mmap_append,
						  .status = ASYNC_PENDING };
}

static AsyncStatus poll_ring_append(AsyncResult *result)
{
	RingAppendState *state = (RingAppendState *)result->state;
	FileAdaptiveState *adaptive = state->parameters.adaptive;
	uint64_t bytes = state->parameters.bytes_to_append;
	AsyncStatus final_status = ASYNC_COMPLETED;

	if (!state->ring_initialized) {
		struct io_uring_params params = { 0 };
		params.sq_entries = 1;
		params.cq_entries = 1;

		int ring_fd = (int)syscall2(SYS_io_uring_setup, 1, (long)&params);
		if (ring_fd < 0) {
			result->error = fun_error_result(-ring_fd, "io_uring_setup failed");
			return ASYNC_ERROR;
		}
		state->ring_fd = ring_fd;

		uint64_t sq_size = params.sq_entries * sizeof(struct io_uring_sqe);
		uint64_t cq_size = params.cq_entries * sizeof(struct io_uring_cqe);

		void *sq_mmap =
			(void *)syscall6(SYS_mmap, 0, (long)sq_size, 0x3, 0x11, ring_fd, 0);
		if ((long)sq_mmap < 0) {
			syscall1(SYS_close, ring_fd);
			result->error = fun_error_result(1, "Failed to mmap SQ");
			final_status = ASYNC_ERROR;
			goto cleanup;
		}
		state->sq_ring = sq_mmap;

		void *cq_mmap = (void *)syscall6(SYS_mmap, 0, (long)cq_size, 0x3, 0x11,
										 ring_fd, (long)cq_size);
		if ((long)cq_mmap < 0) {
			syscall2(SYS_munmap, (long)sq_mmap, (long)sq_size);
			syscall1(SYS_close, ring_fd);
			result->error = fun_error_result(1, "Failed to mmap CQ");
			final_status = ASYNC_ERROR;
			goto cleanup;
		}
		state->cq_ring = cq_mmap;
		state->ring_initialized = true;
		return ASYNC_PENDING;
	}

	if (!state->file_opened) {
		int fd = (int)syscall3(SYS_open, (long)state->parameters.file_path,
							   O_WRONLY | O_CREAT | O_APPEND, 0644);
		if (fd < 0) {
			result->error =
				fun_error_result(-fd, "Failed to open file for append");
			final_status = ASYNC_ERROR;
			goto cleanup;
		}
		state->file_fd = fd;
		state->file_opened = true;
		return ASYNC_PENDING;
	}

	if (!state->io_submitted) {
		struct io_uring_sqe *sqe = (struct io_uring_sqe *)state->sq_ring;
		sqe->opcode = IORING_OP_WRITEV;
		sqe->fd = state->file_fd;
		sqe->off = (uint64_t)-1; /* O_APPEND: kernel appends */
		sqe->addr = (uint64_t)(long)state->parameters.input;
		sqe->len = state->parameters.bytes_to_append;
		sqe->user_data = 1;

		long ret = syscall4(SYS_io_uring_enter, state->ring_fd, 1, 1,
							IORING_ENTER_GETEVENTS);
		if (ret < 0) {
			result->error = fun_error_result(-ret, "io_uring_enter failed");
			final_status = ASYNC_ERROR;
			goto cleanup;
		}
		state->io_submitted = true;
		return ASYNC_PENDING;
	}

	struct io_uring_cqe *cqe = (struct io_uring_cqe *)state->cq_ring;
	if (cqe->user_data == 1) {
		if (cqe->res < 0) {
			result->error =
				fun_error_result(-cqe->res, "io_uring append failed");
			final_status = ASYNC_ERROR;
		} else {
			result->error = ERROR_RESULT_NO_ERROR;
		}
	} else {
		return ASYNC_PENDING;
	}

cleanup:
	if (state->cq_ring && state->cq_ring != (void *)-1)
		syscall2(SYS_munmap, (long)state->cq_ring, sizeof(struct io_uring_cqe));
	if (state->sq_ring && state->sq_ring != (void *)-1)
		syscall2(SYS_munmap, (long)state->sq_ring, sizeof(struct io_uring_sqe));
	if (state->ring_fd >= 0)
		syscall1(SYS_close, state->ring_fd);
	if (state->file_fd >= 0)
		syscall1(SYS_close, state->file_fd);
	fun_memory_free((Memory *)&state);
	if (final_status == ASYNC_COMPLETED)
		file_adaptive_update(adaptive, bytes);
	return final_status;
}

static AsyncResult create_ring_append(Append parameters)
{
	MemoryResult mem_result = fun_memory_allocate(sizeof(RingAppendState));
	if (fun_error_is_error(mem_result.error))
		return (AsyncResult){ .status = ASYNC_ERROR,
							  .error = mem_result.error };

	RingAppendState *state = (RingAppendState *)mem_result.value;
	*state = (RingAppendState){ .parameters = parameters,
								.ring_fd = -1,
								.file_fd = -1,
								.sq_ring = NULL,
								.cq_ring = NULL,
								.ring_initialized = false,
								.file_opened = false,
								.io_submitted = false };

	return (AsyncResult){ .state = state,
						  .poll = poll_ring_append,
						  .status = ASYNC_PENDING };
}

AsyncResult fun_append_memory_to_file(Append parameters)
{
	if (!parameters.file_path || !parameters.input)
		return (AsyncResult){ .status = ASYNC_ERROR,
							  .error = ERROR_RESULT_NULL_POINTER };

	if (parameters.bytes_to_append == 0)
		return (AsyncResult){ .status = ASYNC_COMPLETED,
							  .error = ERROR_RESULT_NO_ERROR };

	FileMode mode = parameters.mode;
	if (mode == FILE_MODE_AUTO)
		mode = file_adaptive_choose(parameters.adaptive);

	if (mode == FILE_MODE_RING_BASED)
		return create_ring_append(parameters);

	return create_mmap_append(parameters);
}
