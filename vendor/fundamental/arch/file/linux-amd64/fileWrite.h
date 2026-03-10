#include "file/file.h"
#include "memory/memory.h"

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct {
	Write parameters;
	int file_descriptor;
	void *mapped_address;
	uint64_t adjusted_offset;
	uint64_t original_file_size;
	bool file_extended;
} MMapWriteState;

AsyncStatus poll_mmap_write(AsyncResult *result);