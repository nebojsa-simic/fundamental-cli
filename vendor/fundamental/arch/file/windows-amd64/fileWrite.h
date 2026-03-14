#include "file/file.h"
#include "memory/memory.h"

#include <windows.h>
#include <memoryapi.h>

typedef struct {
	Write parameters;
	HANDLE file_handle;
	HANDLE mapping_handle;
	LPVOID mapped_view;
	uint64_t adjusted_offset;
	uint64_t original_file_size;
	bool file_extended;
} MMapWriteState;

AsyncStatus poll_mmap_write(AsyncResult *result);
AsyncResult create_ring_write(Write parameters);
