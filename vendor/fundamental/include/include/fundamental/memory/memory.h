#ifndef LIBRARY_MEMORY_H
#define LIBRARY_MEMORY_H

#include <stddef.h>
#include <stdint.h>

#include "../error/error.h"

// Define the memory module related types
typedef void *Memory;
// Define the corresponding error type
DEFINE_RESULT_TYPE(Memory);

// Interface
CanReturnError(Memory) fun_memory_allocate(size_t size);
CanReturnError(Memory) fun_memory_reallocate(Memory memory, size_t newSize);
CanReturnError(void) fun_memory_free(Memory *memory);
CanReturnError(void)
	fun_memory_fill(Memory memory, size_t size, uint64_t value);
CanReturnError(size_t) fun_memory_size(Memory memory);
CanReturnError(void)
	fun_memory_copy(const Memory source, const Memory destination,
					size_t sizeInBytes);

#endif // LIBRARY_MEMORY_H
