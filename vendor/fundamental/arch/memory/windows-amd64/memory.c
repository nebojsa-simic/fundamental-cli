#include "memory/memory.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

CanReturnError(Memory) fun_memory_allocate(size_t size)
{
	MemoryResult result;
	HANDLE hHeap = GetProcessHeap();
	result.value = HeapAlloc(hHeap, 0, size);
	if (result.value == NULL) {
		result.error =
			fun_error_result(GetLastError(), "Failed to allocate memory");
	} else {
		result.error = ERROR_RESULT_NO_ERROR;
	}
	return result;
}

CanReturnError(Memory) fun_memory_reallocate(Memory memory, size_t newSize)
{
	MemoryResult result;
	HANDLE hHeap = GetProcessHeap();
	result.value = HeapReAlloc(hHeap, 0, memory, newSize);
	if (result.value == NULL) {
		result.error =
			fun_error_result(GetLastError(), "Failed to reallocate memory");
	} else {
		result.error = ERROR_RESULT_NO_ERROR;
	}
	return result;
}

CanReturnError(void) fun_memory_free(Memory *memory)
{
	voidResult result;
	HANDLE hHeap = GetProcessHeap();
	if (HeapFree(hHeap, 0, *memory)) {
		result.error = ERROR_RESULT_NO_ERROR;
		*memory = NULL;
	} else {
		result.error =
			fun_error_result(GetLastError(), "Failed to free memory");
	}
	return result;
}

CanReturnError(void) fun_memory_fill(Memory memory, size_t size, uint64_t value)
{
	voidResult result;
	// Validate the memory pointer
	if (memory == NULL) {
		result.error =
			fun_error_result(ERROR_INVALID_PARAMETER, "Invalid memory pointer");
		return result;
	}

	HANDLE hHeap = GetProcessHeap();
	// Check if the memory block is valid using HeapValidate
	if (!HeapValidate(hHeap, 0, memory)) {
		result.error = fun_error_result(GetLastError(), "Invalid memory block");
		return result;
	}

	// Calculate how many 64-bit chunks we can fill
	size_t chunkCount = size / sizeof(uint64_t);
	// Calculate leftover bytes if size isn't a multiple of 8
	size_t remainder = size % sizeof(uint64_t);

	// Fill 64-bit chunks
	uint64_t *ptr64 = (uint64_t *)memory;
	for (size_t i = 0; i < chunkCount; i++) {
		ptr64[i] = value;
	}

	// Fill remainder bytes
	if (remainder > 0) {
		uint8_t *remainderPtr = (uint8_t *)(ptr64 + chunkCount);
		// We copy the first 'remainder' bytes from 'value' to the leftover area
		const uint8_t *valueBytes = (const uint8_t *)&value;
		for (size_t j = 0; j < remainder; j++) {
			remainderPtr[j] = valueBytes[j];
		}
	}

	result.error = ERROR_RESULT_NO_ERROR;
	return result;
}

CanReturnError(size_t) fun_memory_size(Memory memory)
{
	size_tResult result;

	if (memory == NULL) {
		result.value = 0;
		result.error = fun_error_result(ERROR_INVALID_PARAMETER,
										"Cannot get size of NULL pointer");
		return result;
	}

	HANDLE hHeap = GetProcessHeap();
	size_t size = HeapSize(hHeap, 0, memory);
	if (size == (size_t)-1) {
		result.value = 0;
		result.error = fun_error_result(1, "Failed to get memory size");
	} else {
		result.value = size;
		result.error = ERROR_RESULT_NO_ERROR;
	}
	return result;
}

CanReturnError(void)
	fun_memory_copy(const Memory source, const Memory destination,
					size_t sizeInBytes)
{
	voidResult result;

	if (destination == NULL || source == NULL) {
		result.error = fun_error_result(22, "Invalid argument: NULL pointer");
		return result;
	}

	// Check for overlap
	if ((destination < source && destination + sizeInBytes > source) ||
		(source < destination && source + sizeInBytes > destination)) {
		// Handle overlapping memory regions
		uint8_t *dest = (uint8_t *)destination;
		const uint8_t *src = (const uint8_t *)source;

		if (dest > src) {
			// Copy from end to start
			for (size_t i = sizeInBytes; i > 0; --i) {
				dest[i - 1] = src[i - 1];
			}
		} else {
			// Copy from start to end
			for (size_t i = 0; i < sizeInBytes; ++i) {
				dest[i] = src[i];
			}
		}
	} else {
		// Non-overlapping regions, use optimized copy
		uint8_t *dest = (uint8_t *)destination;
		const uint8_t *src = (const uint8_t *)source;

		// Copy 8 bytes at a time if possible
		while (sizeInBytes >= 8) {
			*(uint64_t *)dest = *(const uint64_t *)src;
			dest += 8;
			src += 8;
			sizeInBytes -= 8;
		}

		// Copy remaining bytes
		while (sizeInBytes > 0) {
			*dest++ = *src++;
			--sizeInBytes;
		}
	}

	result.error = ERROR_RESULT_NO_ERROR;
	return result;
}
