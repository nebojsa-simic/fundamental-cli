#include "fileRead.h"

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define SYS_io_uring_setup 425
#define SYS_io_uring_enter 426
#define SYS_open 2
#define SYS_close 3
#define SYS_read 0
#define SYS_mmap 9
#define SYS_munmap 11

#define IORING_OP_READV 1
#define IORING_OP_READ_FIXED 35

#define IORING_ENTER_GETEVENTS 0x01

#define O_RDONLY 0

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

struct iovec {
	void *iov_base;
	size_t iov_len;
};

typedef struct {
	Read parameters;
	int ring_fd;
	int file_fd;
	void *sq_ring;
	void *cq_ring;
	uint32_t ring_mask;
	uint32_t ring_entries;
	bool ring_initialized;
	bool file_opened;
	bool io_submitted;
	void *mapped_buffer;
} RingReadState;

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

static inline long syscall5(long n, long a1, long a2, long a3, long a4, long a5)
{
	long ret;
	register long r10 __asm__("r10") = a4;
	register long r8 __asm__("r8") = a5;
	__asm__ __volatile__("syscall"
						 : "=a"(ret)
						 : "a"(n), "D"(a1), "S"(a2), "d"(a3), "r"(r10), "r"(r8)
						 : "rcx", "r11", "memory");
	return ret;
}

static AsyncStatus poll_io_ring(AsyncResult *result)
{
	RingReadState *state = (RingReadState *)result->state;
	AsyncStatus final_status = ASYNC_COMPLETED;

	if (!state->ring_initialized) {
		struct io_uring_params params = { 0 };
		params.sq_entries = 1;
		params.cq_entries = 1;

		int ring_fd = (int)syscall2(SYS_io_uring_setup, 1, (long)&params);
		if (ring_fd < 0) {
			result->error = fun_error_result(
				-ring_fd, "io_uring_setup failed - kernel too old or disabled");
			return ASYNC_ERROR;
		}
		state->ring_fd = ring_fd;

		uint64_t sq_size = params.sq_entries * sizeof(struct io_uring_sqe);
		uint64_t cq_size = params.cq_entries * sizeof(struct io_uring_cqe);

		void *sq_mmap =
			(void *)syscall6(SYS_mmap, 0, sq_size, 0x3, 0x11, ring_fd, 0);
		if ((long)sq_mmap < 0) {
			syscall1(SYS_close, ring_fd);
			result->error =
				fun_error_result(1, "Failed to mmap submission queue");
			final_status = ASYNC_ERROR;
			goto cleanup;
		}
		state->sq_ring = sq_mmap;

		void *cq_mmap =
			(void *)syscall6(SYS_mmap, 0, cq_size, 0x3, 0x11, ring_fd, cq_size);
		if ((long)cq_mmap < 0) {
			syscall2(SYS_munmap, (long)sq_mmap, sq_size);
			syscall1(SYS_close, ring_fd);
			result->error =
				fun_error_result(1, "Failed to mmap completion queue");
			final_status = ASYNC_ERROR;
			goto cleanup;
		}
		state->cq_ring = cq_mmap;
		state->ring_mask = params.sq_entries - 1;
		state->ring_entries = params.sq_entries;
		state->ring_initialized = true;
		return ASYNC_PENDING;
	}

	if (!state->file_opened) {
		int fd = (int)syscall2(SYS_open, (long)state->parameters.file_path,
							   O_RDONLY);
		if (fd < 0) {
			result->error = fun_error_result(-fd, "Failed to open file");
			final_status = ASYNC_ERROR;
			goto cleanup;
		}
		state->file_fd = fd;
		state->file_opened = true;
		return ASYNC_PENDING;
	}

	if (!state->io_submitted) {
		struct io_uring_sqe *sqe = (struct io_uring_sqe *)state->sq_ring;
		sqe->opcode = IORING_OP_READV;
		sqe->fd = state->file_fd;
		sqe->off = state->parameters.offset;
		sqe->addr = (uint64_t)(long)state->parameters.output;
		sqe->len = state->parameters.bytes_to_read;
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
			result->error = fun_error_result(-cqe->res, "io_uring read failed");
			final_status = ASYNC_ERROR;
		} else {
			result->error = ERROR_RESULT_NO_ERROR;
			final_status = ASYNC_COMPLETED;
		}
	} else {
		return ASYNC_PENDING;
	}

cleanup:
	if (state->cq_ring && state->cq_ring != (void *)-1) {
		uint64_t cq_size = sizeof(struct io_uring_cqe);
		syscall2(SYS_munmap, (long)state->cq_ring, cq_size);
	}
	if (state->sq_ring && state->sq_ring != (void *)-1) {
		uint64_t sq_size = sizeof(struct io_uring_sqe);
		syscall2(SYS_munmap, (long)state->sq_ring, sq_size);
	}
	if (state->ring_fd >= 0) {
		syscall1(SYS_close, state->ring_fd);
	}
	if (state->file_fd >= 0) {
		syscall1(SYS_close, state->file_fd);
	}
	fun_memory_free((Memory *)&state);

	return final_status;
}

static AsyncResult create_ring_read(Read parameters)
{
	MemoryResult mem_result = fun_memory_allocate(sizeof(RingReadState));
	if (fun_error_is_error(mem_result.error)) {
		return (AsyncResult){ .status = ASYNC_ERROR,
							  .error = mem_result.error };
	}

	RingReadState *state = (RingReadState *)mem_result.value;
	*state = (RingReadState){ .parameters = parameters,
							  .ring_fd = -1,
							  .file_fd = -1,
							  .sq_ring = NULL,
							  .cq_ring = NULL,
							  .ring_initialized = false,
							  .file_opened = false,
							  .io_submitted = false,
							  .mapped_buffer = NULL };

	return (AsyncResult){ .state = state,
						  .poll = poll_io_ring,
						  .status = ASYNC_PENDING };
}

AsyncResult fun_read_file_in_memory(Read parameters)
{
	switch (parameters.mode) {
	case FILE_MODE_RING_BASED:
		return create_ring_read(parameters);
	default:
		return (AsyncResult){
			.status = ASYNC_ERROR,
			.error = fun_error_result(
				1, "Ring buffer mode requires FILE_MODE_RING_BASED")
		};
	}
}