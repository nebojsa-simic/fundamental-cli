#include <windows.h>
#include <stdint.h>
#include <stdbool.h>

// ============================================================================
// Wide String Helper
// ============================================================================

static int utf8_to_utf16(const char *utf8, wchar_t *utf16, int utf16_size)
{
	if (utf8 == NULL || utf16 == NULL || utf16_size <= 0) {
		return -1;
	}

	int result = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, utf16, utf16_size);
	if (result == 0) {
		return -1;
	}

	return result;
}

// ============================================================================
// File Size Implementation
// ============================================================================

bool fun_platform_file_size(const char *path, uint64_t *size)
{
	if (path == NULL || size == NULL) {
		return false;
	}

	wchar_t path_wide[MAX_PATH];
	if (utf8_to_utf16(path, path_wide, MAX_PATH) < 0) {
		return false;
	}

	WIN32_FILE_ATTRIBUTE_DATA info;
	if (!GetFileAttributesExW(path_wide, GetFileExInfoStandard, &info)) {
		return false;
	}

	if (info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
		return false;
	}

	*size = ((uint64_t)info.nFileSizeHigh << 32) | (uint64_t)info.nFileSizeLow;
	return true;
}
