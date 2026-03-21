#include "fundamental/filesystem/filesystem.h"
#include <stdbool.h>

// Platform-specific functions (implemented in arch/filesystem/*/file_exists.c)
bool fun_platform_file_exists(const char *path);
bool fun_platform_path_exists(const char *path);

// Platform-specific function from directory.c
bool fun_platform_directory_exists(const char *path);

// ============================================================================
// File Existence Check Implementation
// ============================================================================

boolResult fun_file_exists(Path path)
{
	boolResult result;

	char path_string[512];
	ErrorResult conv =
		fun_path_to_string(path, path_string, sizeof(path_string));
	if (fun_error_is_error(conv)) {
		result.error = conv;
		result.value = false;
		return result;
	}

	result.value = fun_platform_file_exists(path_string);
	result.error = ERROR_RESULT_NO_ERROR;
	return result;
}

// ============================================================================
// Directory Existence Check Implementation
// ============================================================================

boolResult fun_directory_exists(Path path)
{
	boolResult result;

	char path_string[512];
	ErrorResult conv =
		fun_path_to_string(path, path_string, sizeof(path_string));
	if (fun_error_is_error(conv)) {
		result.error = conv;
		result.value = false;
		return result;
	}

	result.value = fun_platform_directory_exists(path_string);
	result.error = ERROR_RESULT_NO_ERROR;
	return result;
}

// ============================================================================
// Generic Path Existence Check Implementation
// ============================================================================

boolResult fun_path_exists(Path path)
{
	boolResult result;

	char path_string[512];
	ErrorResult conv =
		fun_path_to_string(path, path_string, sizeof(path_string));
	if (fun_error_is_error(conv)) {
		result.error = conv;
		result.value = false;
		return result;
	}

	result.value = fun_platform_path_exists(path_string);
	result.error = ERROR_RESULT_NO_ERROR;
	return result;
}
