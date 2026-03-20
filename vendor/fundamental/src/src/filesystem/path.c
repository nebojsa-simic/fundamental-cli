#include "fundamental/filesystem/filesystem.h"
#include "fundamental/filesystem/path.h"
#include <stdbool.h>
#include <stddef.h>

// Platform-specific functions provided by arch layer
char fun_platform_path_separator(void);
int fun_platform_get_working_directory(char *output, size_t output_size);

// Task: Implement fun_path_separator
char fun_path_separator(void)
{
	return fun_platform_path_separator();
}

// Helper: Check if character is a path separator
static bool is_separator(char c)
{
	return c == '/' || c == '\\';
}

// Helper: Check if path is absolute
static bool is_absolute_path(const char *path)
{
	if (path == NULL || path[0] == '\0') {
		return false;
	}

	// Unix absolute path
	if (path[0] == '/') {
		return true;
	}

	// Windows absolute path (e.g., C:\)
	if (path[1] == ':' && (path[2] == '/' || path[2] == '\\')) {
		return true;
	}

	return false;
}

// Helper: Get length of path string
static size_t string_length(const char *s)
{
	if (s == NULL) {
		return 0;
	}
	size_t len = 0;
	while (s[len] != '\0') {
		len++;
	}
	return len;
}

// ==================================================================
// Path Type Implementation
// ==================================================================

ErrorResult fun_path_from_string(char *path, OutputPath output)
{
	if (path == NULL) {
		return fun_error_result(ERROR_CODE_NULL_POINTER, "Path string is NULL");
	}

	if (output == NULL) {
		return fun_error_result(ERROR_CODE_NULL_POINTER, "Output Path is NULL");
	}

	if (output->components == NULL) {
		return fun_error_result(ERROR_CODE_NULL_POINTER,
								"Output Path components array is NULL");
	}

	output->count = 0;
	output->is_absolute = false;

	if (path[0] == '\0') {
		return ERROR_RESULT_NO_ERROR;
	}

	output->is_absolute = is_absolute_path(path);

	size_t path_len = string_length(path);
	size_t start = 0;

	if (output->is_absolute) {
		if (path[0] == '/') {
			start = 1;
		} else if (path[1] == ':') {
			start = 3;
		}
	}

	size_t component_start = start;
	size_t max_components = 256;

	for (size_t i = start; i <= path_len; i++) {
		if (i == path_len || is_separator(path[i])) {
			size_t component_len = i - component_start;

			if (component_len > 0) {
				if (output->count >= max_components) {
					return fun_error_result(ERROR_CODE_BUFFER_TOO_SMALL,
											"Too many path components");
				}

				// Null-terminate this component in the mutable buffer
				if (i < path_len) {
					path[i] = '\0';
				}

				// Preserve . and .. components (normalization is separate)
				output->components[output->count++] = &path[component_start];
			}

			component_start = i + 1;
		}
	}

	return ERROR_RESULT_NO_ERROR;
}

ErrorResult fun_path_to_string(Path path, OutputString output,
							   size_t buffer_size)
{
	if (output == NULL) {
		return fun_error_result(ERROR_CODE_NULL_POINTER,
								"Output buffer is NULL");
	}

	if (buffer_size == 0) {
		return fun_error_result(ERROR_CODE_BUFFER_TOO_SMALL,
								"Buffer size is 0");
	}

	if (path.components == NULL && path.count > 0) {
		return fun_error_result(ERROR_CODE_NULL_POINTER,
								"Path components is NULL");
	}

	output[0] = '\0';

	if (path.count == 0) {
		output[0] = '.';
		output[1] = '\0';
		return ERROR_RESULT_NO_ERROR;
	}

	// Calculate required size first
	size_t required_size = 0;
	if (path.is_absolute) {
		required_size++; // Leading separator
	}

	for (size_t i = 0; i < path.count; i++) {
		const char *component = path.components[i];
		if (component == NULL) {
			continue;
		}
		required_size += string_length(component);
		if (i < path.count - 1) {
			required_size++; // Separator between components
		}
	}

	// Check if buffer is large enough (need +1 for null terminator)
	if (required_size >= buffer_size) {
		return fun_error_result(ERROR_CODE_BUFFER_TOO_SMALL,
								"Buffer too small");
	}

	char sep = fun_path_separator();
	size_t out_pos = 0;

	if (path.is_absolute) {
		output[out_pos++] = sep;
	}

	for (size_t i = 0; i < path.count; i++) {
		const char *component = path.components[i];
		if (component == NULL) {
			continue;
		}

		size_t comp_len = string_length(component);
		for (size_t j = 0; j < comp_len; j++) {
			output[out_pos++] = component[j];
		}

		if (i < path.count - 1) {
			output[out_pos++] = sep;
		}
	}

	output[out_pos] = '\0';
	return ERROR_RESULT_NO_ERROR;
}

ErrorResult fun_path_join(Path base, Path relative, OutputPath output)
{
	if (output == NULL) {
		return fun_error_result(ERROR_CODE_NULL_POINTER, "Output Path is NULL");
	}

	if (output->components == NULL) {
		return fun_error_result(ERROR_CODE_NULL_POINTER,
								"Output Path components array is NULL");
	}

	output->count = 0;
	output->is_absolute = base.is_absolute;

	size_t max_components = 256;

	for (size_t i = 0; i < base.count && output->count < max_components; i++) {
		if (base.components[i] != NULL) {
			output->components[output->count++] = base.components[i];
		}
	}

	for (size_t i = 0; i < relative.count && output->count < max_components;
		 i++) {
		const char *component = relative.components[i];
		if (component == NULL) {
			continue;
		}

		if (component[0] == '.' && component[1] == '\0') {
			continue;
		}

		if (component[0] == '.' && component[1] == '.' &&
			component[2] == '\0') {
			if (output->count > 0) {
				output->count--;
			}
		} else {
			output->components[output->count++] = component;
		}
	}

	return ERROR_RESULT_NO_ERROR;
}

ErrorResult fun_path_normalize(Path path, OutputPath output)
{
	if (output == NULL) {
		return fun_error_result(ERROR_CODE_NULL_POINTER, "Output Path is NULL");
	}

	if (output->components == NULL) {
		return fun_error_result(ERROR_CODE_NULL_POINTER,
								"Output Path components array is NULL");
	}

	output->count = 0;
	output->is_absolute = path.is_absolute;

	size_t max_components = 256;
	const char *temp_components[256];
	size_t temp_count = 0;

	for (size_t i = 0; i < path.count && temp_count < max_components; i++) {
		const char *component = path.components[i];
		if (component == NULL) {
			continue;
		}

		if (component[0] == '.' && component[1] == '\0') {
			continue;
		}

		if (component[0] == '.' && component[1] == '.' &&
			component[2] == '\0') {
			if (temp_count > 0) {
				temp_count--;
			} else if (!path.is_absolute) {
				temp_components[temp_count++] = component;
			}
		} else {
			temp_components[temp_count++] = component;
		}
	}

	for (size_t i = 0; i < temp_count && output->count < max_components; i++) {
		output->components[output->count++] = temp_components[i];
	}

	return ERROR_RESULT_NO_ERROR;
}

ErrorResult fun_path_get_parent(Path path, OutputPath output)
{
	if (output == NULL) {
		return fun_error_result(ERROR_CODE_NULL_POINTER, "Output Path is NULL");
	}

	if (output->components == NULL) {
		return fun_error_result(ERROR_CODE_NULL_POINTER,
								"Output Path components array is NULL");
	}

	output->count = 0;
	output->is_absolute = path.is_absolute;

	if (path.count == 0) {
		return ERROR_RESULT_NO_ERROR;
	}

	size_t max_components = 256;
	size_t parent_count = (path.count > 1) ? path.count - 1 : 0;

	for (size_t i = 0; i < parent_count && output->count < max_components;
		 i++) {
		output->components[output->count++] = path.components[i];
	}

	return ERROR_RESULT_NO_ERROR;
}

ErrorResult fun_path_get_filename(Path path, OutputPath output)
{
	if (output == NULL) {
		return fun_error_result(ERROR_CODE_NULL_POINTER, "Output Path is NULL");
	}

	if (output->components == NULL) {
		return fun_error_result(ERROR_CODE_NULL_POINTER,
								"Output Path components array is NULL");
	}

	output->count = 0;
	output->is_absolute = path.is_absolute;

	if (path.count == 0) {
		return ERROR_RESULT_NO_ERROR;
	}

	output->components[0] = path.components[path.count - 1];
	output->count = 1;

	return ERROR_RESULT_NO_ERROR;
}

PathComponent fun_path_get_component(Path path, size_t index)
{
	if (index >= path.count) {
		return NULL;
	}

	return path.components[index];
}

size_t fun_path_component_count(Path path)
{
	return path.count;
}

bool fun_path_is_valid(Path path)
{
	if (path.components == NULL) {
		return false;
	}

	for (size_t i = 0; i < path.count; i++) {
		if (path.components[i] == NULL) {
			return false;
		}

		if (path.components[i][0] == '\0') {
			return false;
		}
	}

	return true;
}

// Task 3.7: Implement fun_filesystem_get_working_directory
ErrorResult fun_filesystem_get_working_directory(OutputString output)
{
	if (output == NULL) {
		return fun_error_result(ERROR_CODE_NULL_POINTER,
								"Output buffer is NULL");
	}

	int result = fun_platform_get_working_directory(output, 512);
	if (result < 0) {
		output[0] = '\0';
		return fun_error_result(ERROR_CODE_PATH_INVALID,
								"Failed to get working directory");
	}

	return ERROR_RESULT_NO_ERROR;
}
