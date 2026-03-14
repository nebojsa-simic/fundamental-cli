#include <windows.h>
#include <stdbool.h>

// ============================================================================
// Wide String Helpers (no stdlib dependencies)
// ============================================================================

// Get length of wide string
static int fun_wide_string_length(const wchar_t *s)
{
	if (s == NULL) {
		return 0;
	}
	int len = 0;
	while (s[len] != L'\0') {
		len++;
	}
	return len;
}

// Convert UTF-8 string to UTF-16 wide string
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
// File Existence Check Implementation
// ============================================================================

bool fun_platform_file_exists(const char *path)
{
	if (path == NULL) {
		return false;
	}

	wchar_t path_wide[MAX_PATH];
	if (utf8_to_utf16(path, path_wide, MAX_PATH) < 0) {
		return false;
	}

	DWORD attrs = GetFileAttributesW(path_wide);
	if (attrs == INVALID_FILE_ATTRIBUTES) {
		return false;
	}

	// Check if it's NOT a directory (i.e., it's a file)
	return (attrs & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

// ============================================================================
// Generic Path Existence Check Implementation
// ============================================================================

bool fun_platform_path_exists(const char *path)
{
	if (path == NULL) {
		return false;
	}

	wchar_t path_wide[MAX_PATH];
	if (utf8_to_utf16(path, path_wide, MAX_PATH) < 0) {
		return false;
	}

	DWORD attrs = GetFileAttributesW(path_wide);
	if (attrs == INVALID_FILE_ATTRIBUTES) {
		return false;
	}

	// Accept any valid path (file or directory)
	return true;
}
