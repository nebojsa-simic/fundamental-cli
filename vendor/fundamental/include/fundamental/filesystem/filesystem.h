#ifndef LIBRARY_FILESYSTEM_H
#define LIBRARY_FILESYSTEM_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "../memory/memory.h"
#include "../string/string.h"
#include "../error/error.h"
#include "path.h"

// ------------------------------------------------------------------
// Filesystem Module Core Types
// ------------------------------------------------------------------

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
 * Path path;
 * const char *components[] = {"tmp", "test", "nested", "dir"};
 * path.components = components;
 * path.count = 4;
 * path.is_absolute = true;
 * fun_filesystem_create_directory(path);
 */
ErrorResult fun_filesystem_create_directory(Path path);

/**
 * Remove empty directory (fails if directory is not empty)
 * 
 * @param path REQUIRED - Directory path to remove
 * 
 * @return ErrorResult with operation status
 * 
 * Example:
 * Path path;
 * const char *components[] = {"tmp", "test", "dir"};
 * path.components = components;
 * path.count = 3;
 * path.is_absolute = true;
 * fun_filesystem_remove_directory(path);
 */
ErrorResult fun_filesystem_remove_directory(Path path);

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
 * Path path;
 * const char *components[] = {"tmp"};
 * path.components = components;
 * path.count = 1;
 * path.is_absolute = true;
 * Memory buffer = fun_memory_allocate(4096);
 * fun_filesystem_list_directory(path, buffer);
 */
ErrorResult fun_filesystem_list_directory(Path path, Memory output);

// ------------------------------------------------------------------
// Path Utilities
// ------------------------------------------------------------------

/**
 * Join two Paths into a single Path
 * 
 * @param base REQUIRED - Base path
 * @param relative REQUIRED - Relative path component to append
 * @param output REQUIRED - Pre-allocated Path structure to store result
 * 
 * @return ErrorResult with operation status
 * 
 * Example:
 * Path base, relative, output;
 * const char *base_components[] = {"home", "user"};
 * const char *rel_components[] = {"documents"};
 * const char *out_components[16];
 * base.components = base_components;
 * base.count = 2;
 * base.is_absolute = true;
 * relative.components = rel_components;
 * relative.count = 1;
 * relative.is_absolute = false;
 * output.components = out_components;
 * fun_path_join(base, relative, &output);
 */
ErrorResult fun_path_join(Path base, Path relative, OutputPath output);

/**
 * Normalize path by resolving . and .. components
 * 
 * @param path REQUIRED - Path to normalize
 * @param output REQUIRED - Pre-allocated Path structure to store normalized path
 * 
 * @return ErrorResult with operation status
 * 
 * Example:
 * Path path, normalized;
 * const char *components[] = {"home", "user", "..", "user", "documents"};
 * const char *out_components[16];
 * path.components = components;
 * path.count = 5;
 * path.is_absolute = true;
 * normalized.components = out_components;
 * fun_path_normalize(path, &normalized);
 */
ErrorResult fun_path_normalize(Path path, OutputPath output);

/**
 * Extract parent directory from path
 * 
 * @param path REQUIRED - Path to get parent of
 * @param output REQUIRED - Pre-allocated Path structure to store parent path
 * 
 * @return ErrorResult with operation status
 * 
 * Example:
 * Path path, parent;
 * const char *components[] = {"home", "user", "documents", "file.txt"};
 * const char *out_components[16];
 * path.components = components;
 * path.count = 4;
 * path.is_absolute = true;
 * parent.components = out_components;
 * fun_path_get_parent(path, &parent);
 */
ErrorResult fun_path_get_parent(Path path, OutputPath output);

/**
 * Extract filename component from path
 * 
 * @param path REQUIRED - Path to get filename from
 * @param output REQUIRED - Pre-allocated Path structure to store filename
 * 
 * @return ErrorResult with operation status
 * 
 * Example:
 * Path path, filename;
 * const char *components[] = {"home", "user", "documents", "file.txt"};
 * const char *out_components[1];
 * path.components = components;
 * path.count = 4;
 * path.is_absolute = true;
 * filename.components = out_components;
 * fun_path_get_filename(path, &filename);
 */
ErrorResult fun_path_get_filename(Path path, OutputPath output);

/**
 * Convert a string to a Path structure
 * 
 * @param path REQUIRED - String path to parse
 * @param output REQUIRED - Pre-allocated Path structure to populate
 * 
 * @return ErrorResult with operation status
 * 
 * Example:
 * Path path;
 * const char *components[16];
 * path.components = components;
 * fun_path_from_string("/home/user/docs", &path);
 */
ErrorResult fun_path_from_string(char *path, OutputPath output);

/**
 * Convert a Path structure to a null-terminated string
 * 
 * @param path REQUIRED - Path to convert
 * @param output REQUIRED - Buffer to store result string
 * @param buffer_size REQUIRED - Size of output buffer in bytes
 * 
 * @return ErrorResult with operation status
 * 
 * Example:
 * char buffer[512];
 * Path path = ...;
 * fun_path_to_string(path, buffer, sizeof(buffer));
 */
ErrorResult fun_path_to_string(Path path, OutputString output,
							   size_t buffer_size);

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
 * Path path;
 * const char *components[] = {"home", "user", "file.txt"};
 * path.components = components;
 * path.count = 3;
 * path.is_absolute = true;
 * boolResult result = fun_file_exists(path);
 * if (fun_error_is_ok(result.error) && result.value) {
 *     // File exists
 * }
 */
boolResult fun_file_exists(Path path);

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
 * Path path;
 * const char *components[] = {"home", "user", "documents"};
 * path.components = components;
 * path.count = 3;
 * path.is_absolute = true;
 * boolResult result = fun_directory_exists(path);
 * if (fun_error_is_ok(result.error) && result.value) {
 *     // Directory exists
 * }
 */
boolResult fun_directory_exists(Path path);

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
 * Path path;
 * const char *components[] = {"home", "user", "documents"};
 * path.components = components;
 * path.count = 3;
 * path.is_absolute = true;
 * boolResult result = fun_path_exists(path);
 * if (fun_error_is_ok(result.error) && result.value) {
 *     // Path exists (either file or directory)
 * }
 */
boolResult fun_path_exists(Path path);

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
