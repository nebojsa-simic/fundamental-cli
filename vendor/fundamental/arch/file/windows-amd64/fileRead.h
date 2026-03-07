#include "file/file.h"
#include "memory/memory.h"

#include <windows.h>
#include <memoryapi.h>

typedef struct {
	Read parameters;
	HANDLE file_handle;
	HANDLE mapping_handle;
	LPVOID mapped_view;
	uint64_t adjusted_offset;
} MMapState;

AsyncStatus poll_mmap(AsyncResult *result);