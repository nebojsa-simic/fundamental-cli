#include <sys/stat.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>

// ============================================================================
// File Existence Check Implementation (POSIX)
// ============================================================================

bool fun_platform_file_exists(const char *path)
{
	if (path == NULL) {
		return false;
	}

	struct stat st;
	if (stat(path, &st) != 0) {
		return false;
	}

	// Check if it's NOT a directory (i.e., it's a file)
	// Using S_ISREG for regular file, but also accept other non-directory types
	return !S_ISDIR(st.st_mode);
}

bool fun_platform_path_exists(const char *path)
{
	if (path == NULL) {
		return false;
	}

	struct stat st;
	if (stat(path, &st) != 0) {
		return false;
	}

	// Accept any valid path (file or directory)
	return true;
}
