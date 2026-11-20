#include "fundamental/file/file.h"
#include "fileAdaptive.h"
#include "fundamental/memory/memory.h"
#include "fundamental/error/error.h"
#include "syscall_nums.h"
#include "page_size.h"
#include "ring_layout.h"

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef unsigned long size_t;
typedef long off_t;

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

/* io_uring struct definitions are in ring_layout.h */

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

/* Ring append: io_uring_setup → open(O_APPEND) → write(off=-1) → poll cqe */
typedef struct {
	Append parameters;
	int ring_fd;
	int file_fd;
	void *sq_ring;
	void *cq_ring;
	void *sqes;
	uint64_t sq_ring_size;
	uint64_t cq_ring_size;
	uint64_t sqes_size;
	uint32_t *sq_tail;
	uint32_t *sq_mask;
	uint32_t *sq_array;
	uint32_t *cq_head;
	uint32_t *cq_tail_ptr;
	uint32_t *cq_mask;
	struct io_uring_cqe *cqes_base;
	bool ring_initialized;
	bool file_opened;
	bool io_submitted;
	bool cqe_consumed;
	uint64_t bytes_transferred;
} RingAppendState;

/* Write memory barrier — ensures prior stores are visible before head update. */
#define smp_store_release(p, v)                \
	do {                                       \
		__asm__ __volatile__("" ::: "memory"); \
		*(p) = (v);                            \
	} while (0)

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
		if (state->parameters.bytes_to_append > UINT64_MAX - append_offset) {
			result->error = ERROR_RESULT_INTEGER_OVERFLOW;
			final_status = ASYNC_ERROR;
			goto cleanup;
		}
		uint64_t new_size = append_offset + state->parameters.bytes_to_append;
		if (syscall2(SYS_ftruncate, state->file_descriptor, (long)new_size) <
			0) {
			result->error = fun_error_result(1, "Failed to extend file");
			final_status = ASYNC_ERROR;
			goto cleanup;
		}
		/* store alignment info for the mmap step */
		state->aligned_offset =
			(append_offset / get_page_size()) * get_page_size();
		uint64_t intra_page_offset = append_offset - state->aligned_offset;
		if (state->parameters.bytes_to_append >
			UINT64_MAX - intra_page_offset) {
			result->error = ERROR_RESULT_INTEGER_OVERFLOW;
			final_status = ASYNC_ERROR;
			goto cleanup;
		}
		state->view_size =
			state->parameters.bytes_to_append + intra_page_offset;
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

	if (state->parameters.durability_mode == FILE_DURABILITY_SYNC ||
		state->parameters.durability_mode == FILE_DURABILITY_FULL) {
		if (syscall1(SYS_fsync, state->file_descriptor) < 0) {
			result->error = fun_error_result(1, "fsync failed");
			final_status = ASYNC_ERROR;
		}
	}

cleanup:
	if (state->mapped)
		syscall2(SYS_munmap, (long)state->mapped_address,
				 (long)state->view_size);
	if (state->file_opened)
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

		int ring_fd = (int)syscall2(SYS_io_uring_setup, 1, (long)&params);
		if (ring_fd < 0) {
			result->error = fun_error_result(-ring_fd, "io_uring_setup failed");
			return ASYNC_ERROR;
		}
		state->ring_fd = ring_fd;

		uint64_t sq_ring_size =
			params.sq_off.array + params.sq_entries * sizeof(uint32_t);
		uint64_t cq_ring_size = params.cq_off.cqes +
								params.cq_entries * sizeof(struct io_uring_cqe);
		uint64_t sqes_size = params.sq_entries * sizeof(struct io_uring_sqe);

		state->sq_ring_size = sq_ring_size;
		state->cq_ring_size = cq_ring_size;
		state->sqes_size = sqes_size;

		void *sq_ring = (void *)syscall6(SYS_mmap, 0, (long)sq_ring_size,
										 PROT_READ | PROT_WRITE,
										 MAP_SHARED | MAP_POPULATE, ring_fd,
										 (long)IORING_OFF_SQ_RING);
		if ((long)sq_ring < 0) {
			syscall1(SYS_close, ring_fd);
			result->error = fun_error_result(1, "Failed to mmap SQ ring");
			return ASYNC_ERROR;
		}
		state->sq_ring = sq_ring;

		void *cq_ring = (void *)syscall6(SYS_mmap, 0, (long)cq_ring_size,
										 PROT_READ | PROT_WRITE,
										 MAP_SHARED | MAP_POPULATE, ring_fd,
										 (long)IORING_OFF_CQ_RING);
		if ((long)cq_ring < 0) {
			syscall2(SYS_munmap, (long)sq_ring, (long)sq_ring_size);
			syscall1(SYS_close, ring_fd);
			result->error = fun_error_result(1, "Failed to mmap CQ ring");
			return ASYNC_ERROR;
		}
		state->cq_ring = cq_ring;

		void *sqes = (void *)syscall6(SYS_mmap, 0, (long)sqes_size,
									  PROT_READ | PROT_WRITE,
									  MAP_SHARED | MAP_POPULATE, ring_fd,
									  (long)IORING_OFF_SQES);
		if ((long)sqes < 0) {
			syscall2(SYS_munmap, (long)cq_ring, (long)cq_ring_size);
			syscall2(SYS_munmap, (long)sq_ring, (long)sq_ring_size);
			syscall1(SYS_close, ring_fd);
			result->error = fun_error_result(1, "Failed to mmap SQEs");
			return ASYNC_ERROR;
		}
		state->sqes = sqes;

		state->sq_tail = (uint32_t *)((char *)sq_ring + params.sq_off.tail);
		state->sq_mask =
			(uint32_t *)((char *)sq_ring + params.sq_off.ring_mask);
		state->sq_array = (uint32_t *)((char *)sq_ring + params.sq_off.array);
		state->cq_head = (uint32_t *)((char *)cq_ring + params.cq_off.head);
		state->cq_tail_ptr = (uint32_t *)((char *)cq_ring + params.cq_off.tail);
		state->cq_mask =
			(uint32_t *)((char *)cq_ring + params.cq_off.ring_mask);
		state->cqes_base =
			(struct io_uring_cqe *)((char *)cq_ring + params.cq_off.cqes);

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
		uint64_t bytes_remaining =
			state->parameters.bytes_to_append - state->bytes_transferred;
		const char *buf_ptr =
			(const char *)state->parameters.input + state->bytes_transferred;

		uint32_t sq_tail = *state->sq_tail;
		uint32_t sq_index = sq_tail & *state->sq_mask;

		struct io_uring_sqe *sqe =
			(struct io_uring_sqe *)state->sqes + sq_index;
		sqe->opcode = IORING_OP_WRITE;
		sqe->flags = 0;
		sqe->ioprio = 0;
		sqe->fd = state->file_fd;
		sqe->off = (uint64_t)-1; /* O_APPEND: kernel determines position */
		sqe->addr = (uint64_t)(long)buf_ptr;
		sqe->len = (uint32_t)bytes_remaining;
		sqe->rw_flags = 0;
		sqe->user_data = 1;
		sqe->buf_index = 0;

		state->sq_array[sq_index] = sq_index;
		smp_store_release(state->sq_tail, sq_tail + 1);

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

	if (state->cqe_consumed)
		goto cleanup;

	{
		uint32_t cq_head = *state->cq_head;
		if (cq_head == *state->cq_tail_ptr) {
			return ASYNC_PENDING;
		}

		struct io_uring_cqe *cqe = &state->cqes_base[cq_head & *state->cq_mask];

		if (cqe->flags & IORING_CQE_F_MORE) {
			smp_store_release(state->cq_head, cq_head + 1);
			return ASYNC_PENDING;
		}

		smp_store_release(state->cq_head, cq_head + 1);

		if (cqe->res < 0) {
			result->error =
				fun_error_result(-cqe->res, "io_uring append failed");
			final_status = ASYNC_ERROR;
			goto cleanup;
		}

		state->bytes_transferred += (uint64_t)cqe->res;

		if (state->bytes_transferred < state->parameters.bytes_to_append) {
			/* Partial write — resubmit for the remaining bytes. */
			state->io_submitted = false;
			return ASYNC_PENDING;
		}

		state->cqe_consumed = true;
		result->error = ERROR_RESULT_NO_ERROR;
	}

cleanup:
	if (state->sqes)
		syscall2(SYS_munmap, (long)state->sqes, (long)state->sqes_size);
	if (state->cq_ring)
		syscall2(SYS_munmap, (long)state->cq_ring, (long)state->cq_ring_size);
	if (state->sq_ring)
		syscall2(SYS_munmap, (long)state->sq_ring, (long)state->sq_ring_size);
	if (state->ring_fd >= 0)
		syscall1(SYS_close, state->ring_fd);
	if (state->file_opened)
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
	*state = (RingAppendState){
		.parameters = parameters,
		.ring_fd = -1,
		.file_fd = -1,
		.sq_ring = NULL,
		.cq_ring = NULL,
		.sqes = NULL,
		.sq_ring_size = 0,
		.cq_ring_size = 0,
		.sqes_size = 0,
		.sq_tail = NULL,
		.sq_mask = NULL,
		.sq_array = NULL,
		.cq_head = NULL,
		.cq_tail_ptr = NULL,
		.cq_mask = NULL,
		.cqes_base = NULL,
		.ring_initialized = false,
		.file_opened = false,
		.io_submitted = false,
		.cqe_consumed = false,
		.bytes_transferred = 0,
	};

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
