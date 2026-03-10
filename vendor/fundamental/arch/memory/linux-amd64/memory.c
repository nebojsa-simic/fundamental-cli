#include "memory/memory.h"

// System call numbers
#define SYS_mmap 9
#define SYS_munmap 11
#define SYS_brk 12

// mmap flags
#define PROT_READ 0x1
#define PROT_WRITE 0x2
#define MAP_PRIVATE 0x2
#define MAP_ANONYMOUS 0x20

// Page size
#define PAGE_SIZE 4096

// Inline assembly for syscalls
static inline long syscall1(long n, long a1)
{
	long ret;
	__asm__ __volatile__("syscall"
						 : "=a"(ret)
						 : "a"(n), "D"(a1)
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

CanReturnError(Memory) fun_memory_allocate(size_t size)
{
	MemoryResult result;

	// If size is 0, allocate one page
	if (size == 0) {
		size = PAGE_SIZE;
	} else {
		// Round up to the nearest page size
		size = (size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
	}

	long ret = syscall6(SYS_mmap, 0, size, PROT_READ | PROT_WRITE,
						MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (ret < 0 && ret > -4096) {
		result.value = NULL;
		result.error = fun_error_result(-ret, "Failed to allocate memory");
	} else {
		result.value = (void *)ret;
		result.error = ERROR_RESULT_NO_ERROR;
	}
	return result;
}

CanReturnError(Memory) fun_memory_reallocate(Memory memory, size_t newSize)
{
	MemoryResult result;

	if (memory == NULL) {
		// If memory is NULL, return NULL without allocating
		result.value = NULL;
		result.error = ERROR_RESULT_NO_ERROR;
		return result;
	}

	// Allocate new memory
	result = fun_memory_allocate(newSize);
	if (fun_error_is_ok(result.error)) {
		// Copy old data
		size_t copySize = newSize < PAGE_SIZE ? newSize : PAGE_SIZE;
		for (size_t i = 0; i < copySize; i++) {
			((char *)result.value)[i] = ((char *)memory)[i];
		}

		// Free old memory
		voidResult freeResult = fun_memory_free(&memory);
		if (fun_error_is_error(freeResult.error)) {
			// If free fails, we should still return the new allocation
			// but we might want to log this error somehow
		}
	}

	return result;
}

CanReturnError(void) fun_memory_free(Memory *memory)
{
	voidResult result;
	if (*memory != NULL) {
		long ret = syscall1(SYS_brk, (long)*memory);
		if (ret < 0 && ret > -4096) {
			result.error = fun_error_result(-ret, "Failed to free memory");
		} else {
			*memory = NULL;
			result.error = ERROR_RESULT_NO_ERROR;
		}
	} else {
		result.error = ERROR_RESULT_NO_ERROR;
	}
	return result;
}

CanReturnError(void)
	fun_memory_fill(Memory memory, size_t sizeInBytes, uint64_t value)
{
	voidResult result;
	if (memory == NULL) {
		result.error = fun_error_result(22, "Invalid argument");
		return result;
	}

	// Calculate how many 64-bit chunks we can fill
	size_t chunkCount = sizeInBytes / sizeof(uint64_t);
	// Calculate leftover bytes if size isn't a multiple of 8
	size_t remainder = sizeInBytes % sizeof(uint64_t);

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
		result.error = fun_error_result(22, "Invalid argument");
		return result;
	}
	// We can't easily determine the exact size of an allocated block
	// without additional bookkeeping. For simplicity, we'll return a
	// minimum size (e.g., 4096 bytes, typical page size).
	result.value = 4096;
	result.error = ERROR_RESULT_NO_ERROR;
	return result;
}

CanReturnError(void)
	fun_memory_copy(Memory source, const Memory destination, size_t sizeInBytes)
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
