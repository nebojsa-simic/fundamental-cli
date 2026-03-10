#include "filesystem/filesystem.h"
#include <stdbool.h>

// Platform-specific functions (implemented in arch/filesystem/*/directory.c)
int fun_platform_directory_create(const char *path);
int fun_platform_directory_remove(const char *path);
bool fun_platform_directory_exists(const char *path);
bool fun_platform_directory_is_empty(const char *path);
int fun_platform_directory_list(const char *path, char *buffer,
                                size_t buffer_size);

// Helper: Get length of string
static size_t string_length(const char *s) {
  if (s == NULL) {
    return 0;
  }
  size_t len = 0;
  while (s[len] != '\0') {
    len++;
  }
  return len;
}

// Helper: Copy string
static void string_copy(const char *src, char *dest, size_t dest_size) {
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

// Helper: Check if character is path separator
static bool is_separator(char c) { return c == '/' || c == '\\'; }

// Helper: Find last separator in path
static size_t find_last_separator(const char *path) {
  if (path == NULL) {
    return 0;
  }

  size_t len = string_length(path);
  for (size_t i = len; i > 0; i--) {
    if (is_separator(path[i - 1])) {
      return i - 1;
    }
  }
  return 0;
}

// Task 2.3: Directory existence check helper
static bool directory_exists(const char *path) {
  if (path == NULL) {
    return false;
  }
  return fun_platform_directory_exists(path);
}

// Task 2.6: Directory emptiness check helper
static bool directory_is_empty(const char *path) {
  if (path == NULL) {
    return false;
  }
  return fun_platform_directory_is_empty(path);
}

// Task 2.4: Recursive parent directory creation
static ErrorResult create_parent_directories(const char *path) {
  if (path == NULL) {
    return fun_error_result(ERROR_CODE_NULL_POINTER, "Path is NULL");
  }

  size_t len = string_length(path);
  if (len == 0) {
    return fun_error_result(ERROR_CODE_PATH_INVALID, "Path is empty");
  }

  // Find all separators and create directories incrementally
  char temp[512];
  string_copy(path, temp, 512);

  // Skip leading separator for absolute paths
  size_t start = 0;
  if (len > 0 && is_separator(temp[0])) {
    start = 1;
  }

  // Skip Windows drive letter
  if (len > 2 && temp[1] == ':' && is_separator(temp[2])) {
    start = 3;
  }

  // Find each separator and create directory up to that point
  for (size_t i = start; i < len; i++) {
    if (is_separator(temp[i])) {
      temp[i] = '\0';

      // Try to create directory
      int result = fun_platform_directory_create(temp);
      if (result < 0) {
        // Check if it already exists (that's ok)
        if (!directory_exists(temp)) {
          return fun_error_result(ERROR_CODE_PERMISSION_DENIED,
                                  "Cannot create parent directory");
        }
      }

      temp[i] = '/'; // Restore separator
    }
  }

  return ERROR_RESULT_NO_ERROR;
}

// Task 2.2: Implement fun_filesystem_create_directory with parent directory
// creation
ErrorResult fun_filesystem_create_directory(const char *path) {
  // Task 2.9: NULL parameter validation
  if (path == NULL) {
    return fun_error_result(ERROR_CODE_NULL_POINTER, "Path is NULL");
  }

  // Task 2.10: Error handling with appropriate error codes
  size_t len = string_length(path);
  if (len == 0) {
    return fun_error_result(ERROR_CODE_PATH_INVALID, "Path is empty");
  }

  // Create parent directories first
  ErrorResult parent_result = create_parent_directories(path);
  if (fun_error_is_error(parent_result)) {
    return parent_result;
  }

  // Create the final directory
  int result = fun_platform_directory_create(path);
  if (result < 0) {
    // Check if it already exists
    if (directory_exists(path)) {
      return ERROR_RESULT_NO_ERROR; // Idempotent success
    }
    return fun_error_result(ERROR_CODE_PERMISSION_DENIED,
                            "Cannot create directory");
  }

  return ERROR_RESULT_NO_ERROR;
}

// Task 2.5: Implement fun_filesystem_remove_directory for empty directories
// only
ErrorResult fun_filesystem_remove_directory(const char *path) {
  // Task 2.9: NULL parameter validation
  if (path == NULL) {
    return fun_error_result(ERROR_CODE_NULL_POINTER, "Path is NULL");
  }

  size_t len = string_length(path);
  if (len == 0) {
    return fun_error_result(ERROR_CODE_PATH_INVALID, "Path is empty");
  }

  // Check if directory exists
  if (!directory_exists(path)) {
    return fun_error_result(ERROR_CODE_DIRECTORY_NOT_FOUND,
                            "Directory not found");
  }

  // Task 2.6: Check if directory is empty
  if (!directory_is_empty(path)) {
    return fun_error_result(ERROR_CODE_DIRECTORY_NOT_EMPTY,
                            "Directory is not empty");
  }

  // Remove the directory
  int result = fun_platform_directory_remove(path);
  if (result == -2) {
    return fun_error_result(ERROR_CODE_DIRECTORY_NOT_FOUND,
                            "Directory not found");
  } else if (result == -3) {
    return fun_error_result(ERROR_CODE_NOT_DIRECTORY,
                            "Path is not a directory");
  } else if (result < 0) {
    return fun_error_result(ERROR_CODE_PERMISSION_DENIED,
                            "Cannot remove directory");
  }

  return ERROR_RESULT_NO_ERROR;
}

// Task 2.8: Directory entry enumeration helper
typedef struct {
  char *buffer;
  size_t buffer_size;
  size_t bytes_written;
  size_t entry_count;
} DirectoryListContext;

static void add_entry_to_buffer(DirectoryListContext *ctx, const char *entry) {
  if (ctx == NULL || entry == NULL) {
    return;
  }

  size_t entry_len = string_length(entry);

  // Skip . and .. entries
  if ((entry_len == 1 && entry[0] == '.') ||
      (entry_len == 2 && entry[0] == '.' && entry[1] == '.')) {
    return;
  }

  // Check if we have space (entry + newline + null terminator)
  if (ctx->bytes_written + entry_len + 2 > ctx->buffer_size) {
    return; // Buffer too small
  }

  // Add entry
  for (size_t i = 0; i < entry_len; i++) {
    ctx->buffer[ctx->bytes_written++] = entry[i];
  }
  ctx->buffer[ctx->bytes_written++] = '\n'; // Separator
  ctx->entry_count++;
}

// Task 2.7: Implement fun_filesystem_list_directory with buffer filling
ErrorResult fun_filesystem_list_directory(const char *path, void *output) {
  // Task 2.9: NULL parameter validation
  if (path == NULL) {
    return fun_error_result(ERROR_CODE_NULL_POINTER, "Path is NULL");
  }

  if (output == NULL) {
    return fun_error_result(ERROR_CODE_NULL_POINTER, "Output buffer is NULL");
  }

  size_t len = string_length(path);
  if (len == 0) {
    return fun_error_result(ERROR_CODE_PATH_INVALID, "Path is empty");
  }

  // Check if path exists
  if (!directory_exists(path)) {
    return fun_error_result(ERROR_CODE_DIRECTORY_NOT_FOUND,
                            "Directory not found");
  }

  // Get memory size for buffer
  size_t buffer_size = 0;
  // Memory size would be obtained from memory module
  // For now, assume a reasonable default
  buffer_size = 4096;

  // Call platform-specific listing
  int result = fun_platform_directory_list(path, (char *)output, buffer_size);
  if (result < 0) {
    if (result == -4) {
      return fun_error_result(ERROR_CODE_BUFFER_TOO_SMALL,
                              "Buffer too small for directory listing");
    }
    return fun_error_result(ERROR_CODE_PERMISSION_DENIED,
                            "Cannot read directory");
  }

  return ERROR_RESULT_NO_ERROR;
}
