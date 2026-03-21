#include "fundamental/filesystem/filesystem.h"

// Platform-specific function (implemented in arch/filesystem/*/file_size.c)
bool fun_platform_file_size(const char *path, uint64_t *size);

// ============================================================================
// File Size Implementation
// ============================================================================

CanReturnError(void) fun_file_size(Path path, uint64_t *size)
{
	voidResult result;

	if (size == NULL) {
		result.error = ERROR_RESULT_NULL_POINTER;
		return result;
	}

	char path_string[512];
	ErrorResult conv =
		fun_path_to_string(path, path_string, sizeof(path_string));
	if (fun_error_is_error(conv)) {
		result.error = conv;
		return result;
	}

	if (!fun_platform_file_size(path_string, size)) {
		result.error = ERROR_RESULT_PATH_INVALID;
		return result;
	}

	result.error = ERROR_RESULT_NO_ERROR;
	return result;
}
