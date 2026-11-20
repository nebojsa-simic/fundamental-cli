#include "fileWrite.h"
#include "fileAdaptive.h"
#include "syscall_nums.h"

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

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

/* Write memory barrier — ensures prior stores are visible before head update. */
#define smp_store_release(p, v)                \
	do {                                       \
		__asm__ __volatile__("" ::: "memory"); \
		*(p) = (v);                            \
	} while (0)

static AsyncStatus poll_ring_write(AsyncResult *result)
{
	RingWriteState *state = (RingWriteState *)result->state;
	FileAdaptiveState *adaptive = state->parameters.adaptive;
	uint64_t bytes = state->parameters.bytes_to_write;
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
							   O_WRONLY | O_CREAT | O_TRUNC, 0644);
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
		uint64_t bytes_remaining =
			state->parameters.bytes_to_write - state->bytes_transferred;
		uint64_t write_offset =
			state->parameters.offset + state->bytes_transferred;
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
		sqe->off = write_offset;
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
				fun_error_result(-cqe->res, "io_uring write failed");
			final_status = ASYNC_ERROR;
			goto cleanup;
		}

		state->bytes_transferred += (uint64_t)cqe->res;

		if (state->bytes_transferred < state->parameters.bytes_to_write) {
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

AsyncResult create_ring_write(Write parameters)
{
	MemoryResult mem_result = fun_memory_allocate(sizeof(RingWriteState));
	if (fun_error_is_error(mem_result.error))
		return (AsyncResult){ .status = ASYNC_ERROR,
							  .error = mem_result.error };

	RingWriteState *state = (RingWriteState *)mem_result.value;
	*state = (RingWriteState){
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
						  .poll = poll_ring_write,
						  .status = ASYNC_PENDING };
}
