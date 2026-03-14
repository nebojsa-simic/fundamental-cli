#ifndef LIBRARY_FILESYSTEM_H
#define LIBRARY_FILESYSTEM_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "memory/memory.h"
#include "string/string.h"
#include "error/error.h"

// ------------------------------------------------------------------
// Filesystem Module Core Types
// ------------------------------------------------------------------

// ------------------------------------------------------------------
// Platform-independent path separator
// ------------------------------------------------------------------

/**
 * Get the platform-specific path separator character
 * 
 * @return '/' on POSIX systems, '\\' on Windows
 * 
 * Example:
 * char sep = fun_path_separator();
 * // sep == '\\' on Windows
 * // sep == '/' on Linux/POSIX
 */
char fun_path_separator(void);

// ------------------------------------------------------------------
// Directory Operations
// ------------------------------------------------------------------

/**
 * Create directory including parent directories if they don't exist
 * 
 * @param path REQUIRED - Directory path to create
 * 
 * @return ErrorResult with operation status
 * 
 * Example:
 * fun_filesystem_create_directory("/tmp/test/nested/dir");
 */
ErrorResult fun_filesystem_create_directory(String path);

/**
 * Remove empty directory (fails if directory is not empty)
 * 
 * @param path REQUIRED - Directory path to remove
 * 
 * @return ErrorResult with operation status
 * 
 * Example:
 * fun_filesystem_remove_directory("/tmp/test/dir");
 */
ErrorResult fun_filesystem_remove_directory(String path);

/**
 * List directory contents into caller-allocated buffer
 * 
 * @param path REQUIRED - Directory path to list
 * @param output REQUIRED - Pre-allocated buffer to fill with entry names
 *                         Entries separated by newlines
 * 
 * @return ErrorResult with operation status
 * 
 * Example:
 * Memory buffer = fun_memory_allocate(4096);
 * fun_filesystem_list_directory("/tmp", buffer);
 */
ErrorResult fun_filesystem_list_directory(String path, Memory output);

// ------------------------------------------------------------------
// Path Utilities
// ------------------------------------------------------------------

/**
 * Join two path components with platform-appropriate separator
 * 
 * @param base REQUIRED - Base path
 * @param relative REQUIRED - Relative path component to append
 * @param output REQUIRED - Buffer to store result
 * 
 * @return ErrorResult with operation status
 * 
 * Example:
 * char output[512];
 * fun_path_join("/home/user", "documents", output);
 */
ErrorResult fun_path_join(String base, String relative, OutputString output);

/**
 * Normalize path by resolving . and .. components and collapsing separators
 * 
 * @param path REQUIRED - Path to normalize
 * @param output REQUIRED - Buffer to store normalized path
 * 
 * @return ErrorResult with operation status
 * 
 * Example:
 * char output[512];
 * fun_path_normalize("/home/user/../user/./documents", output);
 */
ErrorResult fun_path_normalize(String path, OutputString output);

/**
 * Extract parent directory from path
 * 
 * @param path REQUIRED - Path to get parent of
 * @param output REQUIRED - Buffer to store parent path
 * 
 * @return ErrorResult with operation status
 * 
 * Example:
 * char output[512];
 * fun_path_get_parent("/home/user/documents/file.txt", output);
 * // output = "/home/user/documents"
 */
ErrorResult fun_path_get_parent(String path, OutputString output);

/**
 * Extract filename component from path
 * 
 * @param path REQUIRED - Path to get filename from
 * @param output REQUIRED - Buffer to store filename
 * 
 * @return ErrorResult with operation status
 * 
 * Example:
 * char output[256];
 * fun_path_get_filename("/home/user/documents/file.txt", output);
 * // output = "file.txt"
 */
ErrorResult fun_path_get_filename(String path, OutputString output);

// ------------------------------------------------------------------
// Path Existence Checks
// ------------------------------------------------------------------

/**
 * Check if a file exists at the specified path
 * 
 * @param path REQUIRED - Path to check
 * 
 * @return boolResult with value=true if file exists, false otherwise.
 *         Error codes: ERROR_CODE_NO_ERROR, ERROR_CODE_NULL_POINTER,
 *         ERROR_CODE_PATH_INVALID, ERROR_CODE_PERMISSION_DENIED
 * 
 * Example:
 * boolResult result = fun_file_exists("/home/user/file.txt");
 * if (fun_error_is_ok(result.error) && result.value) {
 *     // File exists
 * }
 */
boolResult fun_file_exists(String path);

/**
 * Check if a directory exists at the specified path
 * 
 * @param path REQUIRED - Path to check
 * 
 * @return boolResult with value=true if directory exists, false otherwise.
 *         Error codes: ERROR_CODE_NO_ERROR, ERROR_CODE_NULL_POINTER,
 *         ERROR_CODE_PATH_INVALID, ERROR_CODE_PERMISSION_DENIED
 * 
 * Example:
 * boolResult result = fun_directory_exists("/home/user/documents");
 * if (fun_error_is_ok(result.error) && result.value) {
 *     // Directory exists
 * }
 */
boolResult fun_directory_exists(String path);

/**
 * Check if any path (file or directory) exists at the specified path
 * 
 * @param path REQUIRED - Path to check
 * 
 * @return boolResult with value=true if path exists, false otherwise.
 *         Error codes: ERROR_CODE_NO_ERROR, ERROR_CODE_NULL_POINTER,
 *         ERROR_CODE_PATH_INVALID, ERROR_CODE_PERMISSION_DENIED
 * 
 * Example:
 * boolResult result = fun_path_exists("/home/user/documents");
 * if (fun_error_is_ok(result.error) && result.value) {
 *     // Path exists (either file or directory)
 * }
 */
boolResult fun_path_exists(String path);

/**
 * Get the current working directory
 *
 * @param output REQUIRED - Buffer to store the working directory path
 *
 * @return ErrorResult with operation status
 *
 * Example:
 * char cwd[512];
 * ErrorResult result = fun_filesystem_get_working_directory(cwd);
 * if (fun_error_is_ok(result)) {
 *     // cwd contains the current working directory
 * }
 */
ErrorResult fun_filesystem_get_working_directory(OutputString output);

#endif // LIBRARY_FILESYSTEM_H
