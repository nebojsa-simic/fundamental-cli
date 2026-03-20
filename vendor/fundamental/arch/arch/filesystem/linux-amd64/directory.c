#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>

#include "fundamental/string/string.h"

// Check if directory exists
static bool directory_exists_posix(const char *path)
{
	struct stat st;
	if (stat(path, &st) != 0) {
		return false;
	}
	return S_ISDIR(st.st_mode);
}

// Check if directory is empty
static bool directory_is_empty_posix(const char *path)
{
	DIR *dir = opendir(path);
	if (dir == NULL) {
		return true; // Can't open, treat as empty
	}

	struct dirent *entry;
	bool empty = true;

	while ((entry = readdir(dir)) != NULL) {
		// Skip . and ..
		if (fun_string_compare(entry->d_name, ".") == 0 ||
			fun_string_compare(entry->d_name, "..") == 0) {
			continue;
		}
		empty = false;
		break;
	}

	closedir(dir);
	return empty;
}

// Create directory (single level, no parents)
static int create_directory_single_posix(const char *path)
{
	// Use 0755 permissions (rwxr-xr-x)
	if (mkdir(path, 0755) == 0) {
		return 0; // Success
	}

	if (errno == EEXIST) {
		// Check if it's a directory
		if (directory_exists_posix(path)) {
			return 0; // Already exists as directory
		}
		return -1; // Exists as file
	}

	return -1; // Other error
}

// Remove directory
static int remove_directory_posix(const char *path)
{
	if (!directory_exists_posix(path)) {
		return -2; // Not found
	}

	struct stat st;
	if (stat(path, &st) != 0 || !S_ISDIR(st.st_mode)) {
		return -3; // Not a directory
	}

	if (!directory_is_empty_posix(path)) {
		return -1; // Not empty
	}

	if (rmdir(path) == 0) {
		return 0; // Success
	}

	return -4; // Remove failed
}

// List directory contents into buffer (newline-separated entries)
int fun_platform_directory_list(const char *path, char *buffer,
								size_t buffer_size)
{
	if (path == NULL || buffer == NULL || buffer_size == 0) {
		return -1;
	}

	DIR *dir = opendir(path);
	if (dir == NULL) {
		return -3; // Can't open directory
	}

	size_t bytes_written = 0;
	size_t max_bytes = buffer_size - 1;

	struct dirent *entry;
	while ((entry = readdir(dir)) != NULL) {
		// Skip . and ..
		if (fun_string_compare(entry->d_name, ".") == 0 ||
			fun_string_compare(entry->d_name, "..") == 0) {
			continue;
		}

		size_t entry_len = fun_string_length(entry->d_name);

		// Check if we have space (entry + newline)
		if (bytes_written + entry_len + 1 >= max_bytes) {
			closedir(dir);
			return -4; // Buffer too small
		}

		// Add entry to buffer
		for (size_t i = 0; i < entry_len; i++) {
			buffer[bytes_written++] = entry->d_name[i];
		}
		buffer[bytes_written++] = '\n';
	}

	closedir(dir);

	// Null-terminate (replace last newline if present)
	if (bytes_written > 0) {
		bytes_written--; // Remove last newline
	}
	buffer[bytes_written] = '\0';

	return (int)bytes_written;
}

// Public POSIX-specific functions
int fun_platform_directory_create(const char *path)
{
	return create_directory_single_posix(path);
}

int fun_platform_directory_remove(const char *path)
{
	return remove_directory_posix(path);
}

bool fun_platform_directory_exists(const char *path)
{
	return directory_exists_posix(path);
}

bool fun_platform_directory_is_empty(const char *path)
{
	return directory_is_empty_posix(path);
}
