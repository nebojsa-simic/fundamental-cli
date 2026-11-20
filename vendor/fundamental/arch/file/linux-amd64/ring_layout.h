#pragma once
#include <stdint.h>
#include <stdbool.h>

/*
 * io_uring ring memory-map offsets (from linux/io_uring.h).
 */
#define IORING_OFF_SQ_RING 0ULL
#define IORING_OFF_CQ_RING 0x8000000ULL
#define IORING_OFF_SQES 0x10000000ULL

/*
 * Byte offsets within the mapped SQ / CQ ring pages, as returned by
 * io_uring_setup() in io_uring_params.sq_off / cq_off.
 */
struct io_sqring_offsets {
	uint32_t head;
	uint32_t tail;
	uint32_t ring_mask;
	uint32_t ring_entries;
	uint32_t flags;
	uint32_t dropped;
	uint32_t array;
	uint32_t resv[3];
};

struct io_cqring_offsets {
	uint32_t head;
	uint32_t tail;
	uint32_t ring_mask;
	uint32_t ring_entries;
	uint32_t overflow;
	uint32_t cqes;
	uint32_t flags;
	uint32_t resv[3];
};

/*
 * Submission Queue Entry.
 */
struct io_uring_sqe {
	uint8_t opcode;
	uint8_t flags;
	uint16_t ioprio;
	int32_t fd;
	uint64_t off;
	uint64_t addr;
	uint32_t len;
	int32_t rw_flags;
	uint64_t user_data;
	uint16_t buf_index;
	uint16_t __pad2[3];
};

/*
 * Completion Queue Entry.
 */
struct io_uring_cqe {
	uint64_t user_data;
	int32_t res;
	uint32_t flags;
};

/*
 * Parameters passed to and filled by io_uring_setup().
 */
struct io_uring_params {
	uint32_t sq_entries;
	uint32_t cq_entries;
	uint32_t flags;
	uint32_t sq_thread_cpu;
	uint32_t sq_thread_idle;
	uint32_t features;
	uint32_t wq_fd;
	uint32_t resv[3];
	struct io_sqring_offsets sq_off;
	struct io_cqring_offsets cq_off;
};
