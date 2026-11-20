#include <stdint.h>
#include <windows.h>

void *_gguf_platform_open(const char *path, uint64_t *out_size,
						  void *handles[2])
{
	HANDLE hFile = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL,
							   OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return NULL;

	LARGE_INTEGER li;
	if (!GetFileSizeEx(hFile, &li)) {
		CloseHandle(hFile);
		return NULL;
	}
	*out_size = (uint64_t)li.QuadPart;

	HANDLE hMapping =
		CreateFileMappingA(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
	if (!hMapping) {
		CloseHandle(hFile);
		return NULL;
	}

	void *view = MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0);
	if (!view) {
		CloseHandle(hMapping);
		CloseHandle(hFile);
		return NULL;
	}

	handles[0] = (void *)hFile;
	handles[1] = (void *)hMapping;
	return view;
}

void _gguf_platform_close(void *data, void *handles[2])
{
	if (data)
		UnmapViewOfFile(data);
	if (handles[1])
		CloseHandle((HANDLE)handles[1]);
	if (handles[0])
		CloseHandle((HANDLE)handles[0]);
}
