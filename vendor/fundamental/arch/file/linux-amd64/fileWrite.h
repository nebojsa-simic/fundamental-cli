#include "fundamental/file/file.h"
#include "fundamental/memory/memory.h"
#include "ring_layout.h"

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct {
	Write parameters;
	int file_descriptor;
	bool fd_valid;
	void *mapped_address;
	bool mmap_valid;
	uint64_t adjusted_offset;
	uint64_t original_file_size;
	bool file_extended;
} MMapWriteState;

typedef struct {
	Write parameters;
	/* ring file descriptors */
	int ring_fd;
	int file_fd;
	/* ring mmaps and their sizes (for munmap on cleanup) */
	void *sq_ring;
	void *cq_ring;
	void *sqes;
	uint64_t sq_ring_size;
	uint64_t cq_ring_size;
	uint64_t sqes_size;
	/* pointers derived from ring mmap + params offsets */
	uint32_t *sq_tail;
	uint32_t *sq_mask;
	uint32_t *sq_array;
	uint32_t *cq_head;
	uint32_t *cq_tail_ptr;
	uint32_t *cq_mask;
	struct io_uring_cqe *cqes_base;
	/* state flags */
	bool ring_initialized;
	bool file_opened;
	bool io_submitted;
	bool cqe_consumed;
	/* tracks bytes written so far for partial-completion re-submission */
	uint64_t bytes_transferred;
} RingWriteState;

AsyncStatus poll_mmap_write(AsyncResult *result);
AsyncResult create_ring_write(Write parameters);
