#include "filesystem/filesystem.h"
#include <stdbool.h>

// Platform-specific functions (implemented in arch/filesystem/*/file_exists.c)
bool fun_platform_file_exists(const char *path);
bool fun_platform_path_exists(const char *path);

// Platform-specific function from directory.c
bool fun_platform_directory_exists(const char *path);

// ============================================================================
// File Existence Check Implementation
// ============================================================================

boolResult fun_file_exists(const char *path)
{
	boolResult result;

	// Validate input
	if (path == NULL) {
		result.error =
			fun_error_result(ERROR_CODE_NULL_POINTER, "Path is NULL");
		result.value = false;
		return result;
	}

	// Check if path exists and is a file (not a directory)
	result.value = fun_platform_file_exists(path);
	result.error = ERROR_RESULT_NO_ERROR;
	return result;
}

// ============================================================================
// Directory Existence Check Implementation
// ============================================================================

boolResult fun_directory_exists(const char *path)
{
	boolResult result;

	// Validate input
	if (path == NULL) {
		result.error =
			fun_error_result(ERROR_CODE_NULL_POINTER, "Path is NULL");
		result.value = false;
		return result;
	}

	// Check if path exists and is a directory
	result.value = fun_platform_directory_exists(path);
	result.error = ERROR_RESULT_NO_ERROR;
	return result;
}

// ============================================================================
// Generic Path Existence Check Implementation
// ============================================================================

boolResult fun_path_exists(const char *path)
{
	boolResult result;

	// Validate input
	if (path == NULL) {
		result.error =
			fun_error_result(ERROR_CODE_NULL_POINTER, "Path is NULL");
		result.value = false;
		return result;
	}

	// Check if any path exists (file or directory)
	result.value = fun_platform_path_exists(path);
	result.error = ERROR_RESULT_NO_ERROR;
	return result;
}
