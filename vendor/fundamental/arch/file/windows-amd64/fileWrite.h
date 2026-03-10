#include "file/file.h"
#include "memory/memory.h"

#include <windows.h>
#include <memoryapi.h>

typedef struct {
	Write parameters; // Original write parameters
	HANDLE file_handle; // File handle from CreateFile
	HANDLE mapping_handle; // File mapping handle from CreateFileMapping
	LPVOID mapped_view; // Mapped memory view from MapViewOfFile
	uint64_t adjusted_offset; // Page-aligned offset for mapping
	uint64_t original_file_size; // Original file size before extension
	bool file_extended; // Whether file was extended for this operation
} MMapWriteState;

AsyncStatus poll_mmap_write(AsyncResult *result);
