#include "filesystem/filesystem.h"
#include <stdbool.h>

// Platform-specific separator provided by arch layer
char fun_platform_path_separator(void);

// Task: Implement fun_path_separator
char fun_path_separator(void) { return fun_platform_path_separator(); }

// Helper: Check if character is a path separator
static bool is_separator(char c) { return c == '/' || c == '\\'; }

// Helper: Check if path is absolute
static bool is_absolute_path(String path) {
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
static size_t string_length(String s) {
  if (s == NULL) {
    return 0;
  }
  size_t len = 0;
  while (s[len] != '\0') {
    len++;
  }
  return len;
}

// Helper: Copy string to output buffer
static void string_copy(String src, OutputString dest, size_t dest_size) {
  if (src == NULL || dest == NULL || dest_size == 0) {
    return;
  }

  size_t i = 0;
  while (i < dest_size - 1 && src[i] != '\0') {
    dest[i] = src[i];
    i++;
  }
  dest[i] = '\0';
}

// Task 3.8: Path validation helper
static ErrorResult validate_path(String path) {
  if (path == NULL) {
    return fun_error_result(ERROR_CODE_NULL_POINTER, "Path is NULL");
  }

  if (path[0] == '\0') {
    return fun_error_result(ERROR_CODE_PATH_INVALID, "Path is empty");
  }

  return ERROR_RESULT_NO_ERROR;
}

// Task 3.2: Implement fun_path_join with separator handling
ErrorResult fun_path_join(String base, String relative, OutputString output) {
  if (output == NULL) {
    return fun_error_result(ERROR_CODE_NULL_POINTER, "Output buffer is NULL");
  }

  output[0] = '\0';

  if (base == NULL || relative == NULL) {
    return fun_error_result(ERROR_CODE_NULL_POINTER,
                            "Base or relative path is NULL");
  }

  size_t base_len = string_length(base);
  size_t rel_len = string_length(relative);

  // Get platform separator once
  char sep = fun_path_separator();

  // If relative is absolute, return it unchanged
  if (is_absolute_path(relative)) {
    string_copy(relative, output, 512);
    return ERROR_RESULT_NO_ERROR;
  }

  // If base is empty, return relative unchanged
  if (base_len == 0) {
    string_copy(relative, output, 512);
    return ERROR_RESULT_NO_ERROR;
  }

  // Build joined path
  size_t i = 0;
  size_t max_len = 511;

  // Copy base
  while (i < max_len && base[i] != '\0') {
    output[i] = base[i];
    i++;
  }

  // Add separator if needed
  if (i > 0 && !is_separator(output[i - 1]) && rel_len > 0) {
    output[i++] = sep;
  }

  // Copy relative
  size_t j = 0;
  while (i < max_len && j < rel_len && relative[j] != '\0') {
    output[i++] = relative[j++];
  }

  output[i] = '\0';
  return ERROR_RESULT_NO_ERROR;
}

// Task 3.3 & 3.4: Implement fun_path_normalize resolving . and .. components
ErrorResult fun_path_normalize(String path, OutputString output) {
  if (output == NULL) {
    return fun_error_result(ERROR_CODE_NULL_POINTER, "Output buffer is NULL");
  }

  output[0] = '\0';

  ErrorResult validation = validate_path(path);
  if (fun_error_is_error(validation)) {
    return validation;
  }

  // Simple normalization: collapse separators, handle . and ..
  // For now, implement basic separator collapsing and . removal
  size_t len = string_length(path);
  if (len == 0) {
    return ERROR_RESULT_NO_ERROR;
  }

  // Stack to hold path components
  char *components[256];
  size_t component_count = 0;
  size_t max_components = 256;

  // Temporary buffer for parsing
  char temp[512];
  string_copy(path, temp, 512);

  // Parse components
  char *token = temp;
  char *next_separator;

  while (token != NULL && *token != '\0' && component_count < max_components) {
    // Find next separator
    next_separator = NULL;
    for (size_t i = 0; token[i] != '\0'; i++) {
      if (is_separator(token[i])) {
        next_separator = &token[i];
        break;
      }
    }

    if (next_separator != NULL) {
      *next_separator = '\0';
    }

    // Skip empty components and current directory markers
    if (token[0] != '\0') {
      if (token[0] == '.' && token[1] == '\0') {
        // Skip "." components
      } else if (token[0] == '.' && token[1] == '.' && token[2] == '\0') {
        // Handle ".." - go up one directory
        if (component_count > 0) {
          component_count--;
        }
      } else {
        // Regular component
        components[component_count++] = token;
      }
    }

    if (next_separator != NULL) {
      token = next_separator + 1;
    } else {
      token = NULL;
    }
  }

  // Rebuild path
  size_t out_pos = 0;
  size_t max_len = 511;

  // Get platform separator once
  char sep = fun_path_separator();

  // Handle absolute paths
  bool absolute = is_absolute_path(path);
  if (absolute) {
    // Copy drive letter if present (Windows)
    if (path[1] == ':') {
      output[out_pos++] = path[0];
      output[out_pos++] = ':';
    }
    output[out_pos++] = sep;
  }

  // Add components
  for (size_t i = 0; i < component_count && out_pos < max_len; i++) {
    size_t comp_len = string_length(components[i]);
    for (size_t j = 0; j < comp_len && out_pos < max_len; j++) {
      output[out_pos++] = components[i][j];
    }
    if (i < component_count - 1 && out_pos < max_len) {
      output[out_pos++] = sep;
    }
  }

  // Handle empty result for absolute paths
  if (out_pos == 0 || (absolute && out_pos == 1)) {
    if (absolute) {
      if (path[1] == ':') {
        output[2] = sep;
        output[3] = '\0';
      } else {
        output[0] = sep;
        output[1] = '\0';
      }
    } else {
      output[0] = '.';
      output[1] = '\0';
    }
  } else {
    output[out_pos] = '\0';
  }

  return ERROR_RESULT_NO_ERROR;
}

// Task 3.5: Implement fun_path_get_parent extracting parent component
ErrorResult fun_path_get_parent(String path, OutputString output) {
  if (output == NULL) {
    return fun_error_result(ERROR_CODE_NULL_POINTER, "Output buffer is NULL");
  }

  output[0] = '\0';

  ErrorResult validation = validate_path(path);
  if (fun_error_is_error(validation)) {
    return validation;
  }

  size_t len = string_length(path);
  if (len == 0) {
    output[0] = '.';
    output[1] = '\0';
    return ERROR_RESULT_NO_ERROR;
  }

  // Remove trailing separators
  size_t end = len;
  while (end > 0 && is_separator(path[end - 1])) {
    end--;
  }

  // Find last separator
  size_t last_sep = end;
  while (last_sep > 0 && !is_separator(path[last_sep - 1])) {
    last_sep--;
  }

  // Get platform separator
  char sep = fun_path_separator();

  // Handle root path
  if (last_sep <= 1 && is_absolute_path(path)) {
    // Return root
    if (path[1] == ':') {
      output[0] = path[0];
      output[1] = ':';
      output[2] = sep;
      output[3] = '\0';
    } else {
      output[0] = sep;
      output[1] = '\0';
    }
    return ERROR_RESULT_NO_ERROR;
  }

  // Handle single component (no parent)
  if (last_sep == 0) {
    output[0] = '.';
    output[1] = '\0';
    return ERROR_RESULT_NO_ERROR;
  }

  // Copy parent path
  size_t parent_len = last_sep;
  while (parent_len > 0 && is_separator(path[parent_len - 1])) {
    parent_len--;
  }

  if (parent_len == 0) {
    output[0] = sep;
    output[1] = '\0';
  } else {
    for (size_t i = 0; i < parent_len && i < 511; i++) {
      output[i] = path[i];
    }
    output[parent_len] = '\0';
  }

  return ERROR_RESULT_NO_ERROR;
}

// Task 3.6: Implement fun_path_get_filename extracting filename component
ErrorResult fun_path_get_filename(String path, OutputString output) {
  if (output == NULL) {
    return fun_error_result(ERROR_CODE_NULL_POINTER, "Output buffer is NULL");
  }

  output[0] = '\0';

  ErrorResult validation = validate_path(path);
  if (fun_error_is_error(validation)) {
    return validation;
  }

  size_t len = string_length(path);
  if (len == 0) {
    return ERROR_RESULT_NO_ERROR;
  }

  // Remove trailing separators
  size_t end = len;
  while (end > 0 && is_separator(path[end - 1])) {
    end--;
  }

  // If path ends with separator, no filename
  if (end == 0) {
    return ERROR_RESULT_NO_ERROR;
  }

  // Find last separator
  size_t start = end;
  while (start > 0 && !is_separator(path[start - 1])) {
    start--;
  }

  // Copy filename
  size_t filename_len = end - start;
  if (filename_len > 255) {
    filename_len = 255;
  }

  for (size_t i = 0; i < filename_len; i++) {
    output[i] = path[start + i];
  }
  output[filename_len] = '\0';

  return ERROR_RESULT_NO_ERROR;
}
