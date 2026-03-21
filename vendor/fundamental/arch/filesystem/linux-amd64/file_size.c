#include <sys/stat.h>
#include <stdint.h>
#include <stdbool.h>

// ============================================================================
// File Size Implementation (POSIX)
// ============================================================================

bool fun_platform_file_size(const char *path, uint64_t *size)
{
	if (path == NULL || size == NULL) {
		return false;
	}

	struct stat st;
	if (stat(path, &st) != 0) {
		return false;
	}

	if (S_ISDIR(st.st_mode)) {
		return false;
	}

	*size = (uint64_t)st.st_size;
	return true;
}
