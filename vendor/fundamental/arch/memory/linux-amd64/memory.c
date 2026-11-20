#include "fundamental/memory/memory.h"

// System call numbers
#define SYS_mmap 9
#define SYS_munmap 11
#define SYS_mremap 25

// mmap flags
#define PROT_READ 0x1
#define PROT_WRITE 0x2
#define MAP_PRIVATE 0x2
#define MAP_ANONYMOUS 0x20

// mremap flags
#define MREMAP_MAYMOVE 1

// Page size
#define PAGE_SIZE 4096

// Allocation header stored immediately before user pointer
typedef struct {
	size_t size;
} MemoryBlockHeader;

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

static inline long syscall2(long n, long a1, long a2)
{
	long ret;
	__asm__ __volatile__("syscall"
						 : "=a"(ret)
						 : "a"(n), "D"(a1), "S"(a2)
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

	if (size > (size_t)-1 - sizeof(MemoryBlockHeader) - PAGE_SIZE) {
		result.value = NULL;
		result.error = fun_error_result(12, "Cannot allocate memory");
		return result;
	}

	size_t totalSize = sizeof(MemoryBlockHeader) + size;
	size_t pageSize = (totalSize + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

	long ret = syscall6(SYS_mmap, 0, pageSize, PROT_READ | PROT_WRITE,
						MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (ret < 0 && ret > -4096) {
		result.value = NULL;
		result.error = fun_error_result(-ret, "Failed to allocate memory");
	} else {
		MemoryBlockHeader *hdr = (MemoryBlockHeader *)ret;
		hdr->size = size;
		result.value = (Memory)(hdr + 1);
		result.error = ERROR_RESULT_NO_ERROR;
	}
	return result;
}

CanReturnError(Memory) fun_memory_reallocate(Memory memory, size_t newSize)
{
	MemoryResult result;

	if (memory == NULL) {
		result.value = NULL;
		result.error = ERROR_RESULT_NO_ERROR;
		return result;
	}

	MemoryBlockHeader *hdr = (MemoryBlockHeader *)memory - 1;
	size_t oldTotal = sizeof(MemoryBlockHeader) + hdr->size;
	size_t oldPages = (oldTotal + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
	size_t newTotal = sizeof(MemoryBlockHeader) + newSize;
	size_t newPages = (newTotal + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

	if (oldPages == newPages) {
		hdr->size = newSize;
		result.value = memory;
		result.error = ERROR_RESULT_NO_ERROR;
		return result;
	}

	long ret =
		syscall5(SYS_mremap, (long)hdr, oldPages, newPages, MREMAP_MAYMOVE, 0);
	if (ret < 0 && ret > -4096) {
		result.value = NULL;
		result.error = fun_error_result(-ret, "Failed to reallocate memory");
		return result;
	}

	MemoryBlockHeader *newHdr = (MemoryBlockHeader *)ret;
	newHdr->size = newSize;
	result.value = (Memory)(newHdr + 1);
	result.error = ERROR_RESULT_NO_ERROR;
	return result;
}

CanReturnError(void) fun_memory_free(Memory *memory)
{
	voidResult result;
	if (*memory == NULL) {
		result.error = ERROR_RESULT_NO_ERROR;
		return result;
	}

	MemoryBlockHeader *hdr = (MemoryBlockHeader *)*memory - 1;
	size_t totalSize = sizeof(MemoryBlockHeader) + hdr->size;
	size_t pageSize = (totalSize + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

	long ret = syscall2(SYS_munmap, (long)hdr, pageSize);
	if (ret < 0 && ret > -4096) {
		result.error = fun_error_result(-ret, "Failed to free memory");
	} else {
		*memory = NULL;
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

	size_t chunkCount = sizeInBytes / sizeof(uint64_t);
	size_t remainder = sizeInBytes % sizeof(uint64_t);

	uint64_t *ptr64 = (uint64_t *)memory;
	for (size_t i = 0; i < chunkCount; i++) {
		ptr64[i] = value;
	}

	if (remainder > 0) {
		uint8_t *remainderPtr = (uint8_t *)(ptr64 + chunkCount);
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

	MemoryBlockHeader *hdr = (MemoryBlockHeader *)memory - 1;
	result.value = hdr->size;
	result.error = ERROR_RESULT_NO_ERROR;
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

	if ((destination < source && destination + sizeInBytes > source) ||
		(source < destination && source + sizeInBytes > destination)) {
		uint8_t *dest = (uint8_t *)destination;
		const uint8_t *src = (const uint8_t *)source;

		if (dest > src) {
			for (size_t i = sizeInBytes; i > 0; --i) {
				dest[i - 1] = src[i - 1];
			}
		} else {
			for (size_t i = 0; i < sizeInBytes; ++i) {
				dest[i] = src[i];
			}
		}
	} else {
		uint8_t *dest = (uint8_t *)destination;
		const uint8_t *src = (const uint8_t *)source;

		while (sizeInBytes >= 8) {
			*(uint64_t *)dest = *(const uint64_t *)src;
			dest += 8;
			src += 8;
			sizeInBytes -= 8;
		}

		while (sizeInBytes > 0) {
			*dest++ = *src++;
			--sizeInBytes;
		}
	}

	result.error = ERROR_RESULT_NO_ERROR;
	return result;
}

CanReturnError(int32_t)
	fun_memory_compare(const Memory a, const Memory b, size_t sizeInBytes)
{
	int32_tResult result;
	const unsigned char *pa = (const unsigned char *)a;
	const unsigned char *pb = (const unsigned char *)b;
	for (size_t i = 0; i < sizeInBytes; i++) {
		if (pa[i] < pb[i]) {
			result.value = -1;
			result.error = ERROR_RESULT_NO_ERROR;
			return result;
		}
		if (pa[i] > pb[i]) {
			result.value = 1;
			result.error = ERROR_RESULT_NO_ERROR;
			return result;
		}
	}
	result.value = 0;
	result.error = ERROR_RESULT_NO_ERROR;
	return result;
}
