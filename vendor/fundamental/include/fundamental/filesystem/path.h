#ifndef LIBRARY_PATH_H
#define LIBRARY_PATH_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "../error/error.h"
#include "../string/string.h"
#include "../memory/memory.h"

// ------------------------------------------------------------------
// Path Type Definition
// ------------------------------------------------------------------

/**
 * Path represents a filesystem path as an array of string components
 * without platform-specific separators.
 * 
 * Components are stored as an array of const char* pointers.
 * The is_absolute flag indicates whether the path is absolute.
 * 
 * Example:
 * Path path;
 * path.components = (const char*[]){"home", "user", "documents"};
 * path.count = 3;
 * path.is_absolute = true;
 */
typedef struct {
	const char **components; // Array of path component strings
	size_t count; // Number of components
	bool is_absolute; // Whether path is absolute
} Path;

/**
 * OutputPath is a typedef for Path* to follow library conventions
 * for output parameters.
 */
typedef Path *OutputPath;

/**
 * PathComponent represents a single path component string.
 */
typedef const char *PathComponent;

/**
 * PathResult for error handling with Path return values.
 */
DEFINE_RESULT_TYPE(Path);

// ------------------------------------------------------------------
// Path Conversion Functions
// ------------------------------------------------------------------

/**
 * Convert a null-terminated string to a Path structure.
 * 
 * @param path REQUIRED - String path to parse (e.g., "/home/user/file.txt")
 * @param output REQUIRED - Pre-allocated Path structure to populate
 * 
 * @return ErrorResult with operation status
 * 
 * Example:
 * Path path;
 * const char *components[16];
 * path.components = components;
 * ErrorResult result = fun_path_from_string("/home/user/docs", &path);
 */
ErrorResult fun_path_from_string(String path, OutputPath output);

/**
 * Convert a Path structure to a null-terminated string.
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
 * ErrorResult result = fun_path_to_string(path, buffer, sizeof(buffer));
 */
ErrorResult fun_path_to_string(Path path, OutputString output,
							   size_t buffer_size);

// ------------------------------------------------------------------
// Path Operations
// ------------------------------------------------------------------

/**
 * Join two Paths into a single Path.
 * 
 * @param base REQUIRED - Base path
 * @param relative REQUIRED - Relative path to append
 * @param output REQUIRED - Pre-allocated Path structure for result
 * 
 * @return ErrorResult with operation status
 * 
 * Example:
 * Path base = ...;
 * Path relative = ...;
 * Path result;
 * const char *components[32];
 * result.components = components;
 * fun_path_join(base, relative, &result);
 */
ErrorResult fun_path_join(Path base, Path relative, OutputPath output);

/**
 * Normalize a Path by resolving . and .. components.
 * 
 * @param path REQUIRED - Path to normalize
 * @param output REQUIRED - Pre-allocated Path structure for result
 * 
 * @return ErrorResult with operation status
 * 
 * Example:
 * Path path = ...;
 * Path normalized;
 * const char *components[32];
 * normalized.components = components;
 * fun_path_normalize(path, &normalized);
 */
ErrorResult fun_path_normalize(Path path, OutputPath output);

/**
 * Extract parent directory from a Path.
 * 
 * @param path REQUIRED - Path to get parent of
 * @param output REQUIRED - Pre-allocated Path structure for result
 * 
 * @return ErrorResult with operation status
 * 
 * Example:
 * Path path = ...;
 * Path parent;
 * const char *components[32];
 * parent.components = components;
 * fun_path_get_parent(path, &parent);
 */
ErrorResult fun_path_get_parent(Path path, OutputPath output);

/**
 * Extract filename (last component) from a Path.
 * 
 * @param path REQUIRED - Path to get filename from
 * @param output REQUIRED - Pre-allocated Path structure for result
 * 
 * @return ErrorResult with operation status
 * 
 * Example:
 * Path path = ...;
 * Path filename;
 * const char *components[1];
 * filename.components = components;
 * fun_path_get_filename(path, &filename);
 */
ErrorResult fun_path_get_filename(Path path, OutputPath output);

// ------------------------------------------------------------------
// Path Component Access
// ------------------------------------------------------------------

/**
 * Get component at specified index.
 * 
 * @param path REQUIRED - Path to access
 * @param index REQUIRED - Index of component (0-based)
 * 
 * @return PathComponent at index, NULL if out of bounds
 * 
 * Example:
 * Path path = ...;
 * String component = fun_path_get_component(path, 0);
 */
PathComponent fun_path_get_component(Path path, size_t index);

/**
 * Get number of components in path.
 * 
 * @param path REQUIRED - Path to query
 * 
 * @return Number of components
 * 
 * Example:
 * Path path = ...;
 * size_t count = fun_path_component_count(path);
 */
size_t fun_path_component_count(Path path);

/**
 * Validate Path structure.
 * 
 * @param path REQUIRED - Path to validate
 * 
 * @return true if path is valid (all components non-NULL and non-empty)
 * 
 * Example:
 * Path path = ...;
 * if (fun_path_is_valid(path)) {
 *     // Path is well-formed
 * }
 */
bool fun_path_is_valid(Path path);

#endif // LIBRARY_PATH_H
