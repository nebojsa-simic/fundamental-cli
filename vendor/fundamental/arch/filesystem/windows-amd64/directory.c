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

// Copy wide string
static wchar_t *fun_wide_string_copy(wchar_t *dest, const wchar_t *src)
{
	if (dest == NULL || src == NULL) {
		return dest;
	}

	wchar_t *orig_dest = dest;
	while (*src != L'\0') {
		*dest++ = *src++;
	}
	*dest = L'\0';
	return orig_dest;
}

// Concatenate wide strings
static wchar_t *fun_wide_string_concatenate(wchar_t *dest, const wchar_t *src)
{
	if (dest == NULL || src == NULL) {
		return dest;
	}

	wchar_t *ptr = dest;
	// Find end of dest string
	while (*ptr != L'\0') {
		ptr++;
	}
	// Copy src to end of dest
	while (*src != L'\0') {
		*ptr++ = *src++;
	}
	*ptr = L'\0';
	return dest;
}

// Compare wide strings
static int fun_wide_string_compare(const wchar_t *s1, const wchar_t *s2)
{
	if (s1 == NULL || s2 == NULL) {
		return s1 == s2 ? 0 : (s1 == NULL ? -1 : 1);
	}

	while (*s1 != L'\0' && *s1 == *s2) {
		s1++;
		s2++;
	}
	return *(const unsigned short *)s1 - *(const unsigned short *)s2;
}

// ============================================================================
// UTF-8/UTF-16 Conversion
// ============================================================================

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

// Check if directory exists
static bool directory_exists_wide(const wchar_t *path_wide)
{
	DWORD attrs = GetFileAttributesW(path_wide);
	if (attrs == INVALID_FILE_ATTRIBUTES) {
		return false;
	}
	return (attrs & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

// Check if directory is empty
static bool directory_is_empty_wide(const wchar_t *path_wide)
{
	WIN32_FIND_DATAW find_data;
	wchar_t search_path[MAX_PATH];

	// Build search pattern: path\*
	int len = fun_wide_string_length(path_wide);
	if (len >= MAX_PATH - 2) {
		return false;
	}

	fun_wide_string_copy(search_path, path_wide);
	if (len > 0 && path_wide[len - 1] != L'\\' && path_wide[len - 1] != L'/') {
		fun_wide_string_concatenate(search_path, L"\\");
	}
	fun_wide_string_concatenate(search_path, L"*");

	HANDLE hFind = FindFirstFileW(search_path, &find_data);
	if (hFind == INVALID_HANDLE_VALUE) {
		return true; // Can't read directory, treat as empty
	}

	bool empty = true;
	do {
		// Skip . and ..
		if (fun_wide_string_compare(find_data.cFileName, L".") == 0 ||
			fun_wide_string_compare(find_data.cFileName, L"..") == 0) {
			continue;
		}
		empty = false;
		break;
	} while (FindNextFileW(hFind, &find_data));

	FindClose(hFind);
	return empty;
}

// Create directory (single level, no parents)
static int create_directory_single_wide(const wchar_t *path_wide)
{
	if (CreateDirectoryW(path_wide, NULL)) {
		return 0; // Success
	}

	DWORD error = GetLastError();
	if (error == ERROR_ALREADY_EXISTS) {
		// Check if it's a directory
		DWORD attrs = GetFileAttributesW(path_wide);
		if (attrs != INVALID_FILE_ATTRIBUTES &&
			(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
			return 0; // Already exists as directory
		}
		return -1; // Exists as file
	}

	return -1; // Other error
}

// Remove directory
static int remove_directory_wide(const wchar_t *path_wide)
{
	if (!directory_exists_wide(path_wide)) {
		return -2; // Not found
	}

	DWORD attrs = GetFileAttributesW(path_wide);
	if (!(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
		return -3; // Not a directory
	}

	if (!directory_is_empty_wide(path_wide)) {
		return -1; // Not empty
	}

	if (RemoveDirectoryW(path_wide)) {
		return 0; // Success
	}

	return -4; // Remove failed
}

// Public Windows-specific functions
int fun_platform_directory_create(const char *path)
{
	wchar_t path_wide[MAX_PATH];
	if (utf8_to_utf16(path, path_wide, MAX_PATH) < 0) {
		return -1;
	}

	return create_directory_single_wide(path_wide);
}

int fun_platform_directory_remove(const char *path)
{
	wchar_t path_wide[MAX_PATH];
	if (utf8_to_utf16(path, path_wide, MAX_PATH) < 0) {
		return -1;
	}

	return remove_directory_wide(path_wide);
}

bool fun_platform_directory_exists(const char *path)
{
	wchar_t path_wide[MAX_PATH];
	if (utf8_to_utf16(path, path_wide, MAX_PATH) < 0) {
		return false;
	}

	return directory_exists_wide(path_wide);
}

bool fun_platform_directory_is_empty(const char *path)
{
	wchar_t path_wide[MAX_PATH];
	if (utf8_to_utf16(path, path_wide, MAX_PATH) < 0) {
		return false;
	}

	return directory_is_empty_wide(path_wide);
}

// List directory contents into buffer (newline-separated entries)
int fun_platform_directory_list(const char *path, char *buffer,
								size_t buffer_size)
{
	if (path == NULL || buffer == NULL || buffer_size == 0) {
		return -1;
	}

	wchar_t path_wide[MAX_PATH];
	if (utf8_to_utf16(path, path_wide, MAX_PATH) < 0) {
		return -1;
	}

	// Build search pattern: path\*
	wchar_t search_path[MAX_PATH + 2];
	int len = fun_wide_string_length(path_wide);
	if (len >= MAX_PATH - 2) {
		return -2; // Path too long
	}

	fun_wide_string_copy(search_path, path_wide);
	if (len > 0 && path_wide[len - 1] != L'\\' && path_wide[len - 1] != L'/') {
		fun_wide_string_concatenate(search_path, L"\\");
	}
	fun_wide_string_concatenate(search_path, L"*");

	WIN32_FIND_DATAW find_data;
	HANDLE hFind = FindFirstFileW(search_path, &find_data);
	if (hFind == INVALID_HANDLE_VALUE) {
		return -3; // Can't open directory
	}

	size_t bytes_written = 0;
	size_t max_bytes = buffer_size - 1;

	do {
		// Skip . and ..
		if (fun_wide_string_compare(find_data.cFileName, L".") == 0 ||
			fun_wide_string_compare(find_data.cFileName, L"..") == 0) {
			continue;
		}

		// Convert filename to UTF-8
		char entry_utf8[260];
		int entry_len = WideCharToMultiByte(CP_UTF8, 0, find_data.cFileName, -1,
											entry_utf8, sizeof(entry_utf8),
											NULL, NULL);
		if (entry_len <= 0) {
			continue;
		}

		// Remove null terminator for length calculation
		entry_len--;

		// Determine type prefix: "D\t" for directory, "F\t" for file
		char type_prefix[2];
		type_prefix[0] =
			(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? 'D' : 'F';
		type_prefix[1] = '\t';

		// Check if we have space (prefix + entry + newline)
		if (bytes_written + 2 + entry_len + 1 >= max_bytes) {
			FindClose(hFind);
			return -4; // Buffer too small
		}

		// Add type prefix
		buffer[bytes_written++] = type_prefix[0];
		buffer[bytes_written++] = type_prefix[1];

		// Add entry to buffer
		for (int i = 0; i < entry_len; i++) {
			buffer[bytes_written++] = entry_utf8[i];
		}
		buffer[bytes_written++] = '\n';

	} while (FindNextFileW(hFind, &find_data));

	FindClose(hFind);

	// Null-terminate (replace last newline if present)
	if (bytes_written > 0) {
		bytes_written--; // Remove last newline
	}
	buffer[bytes_written] = '\0';

	return (int)bytes_written;
}

// ============================================================================
// Streaming Directory Handle (Walk API)
// ============================================================================

// Internal layout for Windows handle blob
typedef struct {
	HANDLE hFind;
	WIN32_FIND_DATAW find_data;
	int has_first; // 1 = find_data holds an unread first entry
} WinDirHandle;

// Static assertion that FUN_DIR_HANDLE_SIZE is large enough
typedef char _win_dir_handle_size_check[640 >= sizeof(WinDirHandle) ? 1 : -1];

int fun_platform_dir_open(const char *path, void *handle_buf)
{
	if (path == NULL || handle_buf == NULL) {
		return -1;
	}

	WinDirHandle *h = (WinDirHandle *)handle_buf;

	wchar_t path_wide[MAX_PATH];
	if (utf8_to_utf16(path, path_wide, MAX_PATH) < 0) {
		return -1;
	}

	// Build search pattern: path\*
	wchar_t search_path[MAX_PATH + 2];
	int len = fun_wide_string_length(path_wide);
	if (len >= MAX_PATH - 2) {
		return -1;
	}
	fun_wide_string_copy(search_path, path_wide);
	if (len > 0 && path_wide[len - 1] != L'\\' && path_wide[len - 1] != L'/') {
		fun_wide_string_concatenate(search_path, L"\\");
	}
	fun_wide_string_concatenate(search_path, L"*");

	h->hFind = FindFirstFileW(search_path, &h->find_data);
	if (h->hFind == INVALID_HANDLE_VALUE) {
		h->has_first = 0;
		return -1;
	}
	h->has_first = 1;
	return 0;
}

int fun_platform_dir_read_entry(void *handle_buf, char *name_buf,
								size_t name_buf_size, char *type_out)
{
	if (handle_buf == NULL || name_buf == NULL || type_out == NULL) {
		return -1;
	}

	WinDirHandle *h = (WinDirHandle *)handle_buf;
	if (h->hFind == INVALID_HANDLE_VALUE) {
		return 0;
	}

	while (1) {
		WIN32_FIND_DATAW *fd;
		WIN32_FIND_DATAW next_data;

		if (h->has_first) {
			fd = &h->find_data;
			h->has_first = 0;
		} else {
			if (!FindNextFileW(h->hFind, &next_data)) {
				return 0; // No more entries
			}
			fd = &next_data;
		}

		// Skip . and ..
		if (fun_wide_string_compare(fd->cFileName, L".") == 0 ||
			fun_wide_string_compare(fd->cFileName, L"..") == 0) {
			continue;
		}

		// Convert filename to UTF-8
		int entry_len = WideCharToMultiByte(CP_UTF8, 0, fd->cFileName, -1,
											name_buf, (int)name_buf_size, NULL,
											NULL);
		if (entry_len <= 0) {
			continue;
		}

		// Set type
		*type_out = (fd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? 'D' :
																		'F';
		return 1;
	}
}

void fun_platform_dir_close(void *handle_buf)
{
	if (handle_buf == NULL) {
		return;
	}
	WinDirHandle *h = (WinDirHandle *)handle_buf;
	if (h->hFind != INVALID_HANDLE_VALUE) {
		FindClose(h->hFind);
		h->hFind = INVALID_HANDLE_VALUE;
	}
}
