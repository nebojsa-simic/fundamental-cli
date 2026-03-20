#ifndef LIBRARY_STRING_H
#define LIBRARY_STRING_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "../error/error.h"

// Define the string module related types
typedef uint64_t StringLength;
typedef const char *String;
typedef char *OutputString;
typedef int64_t StringPosition;
typedef int8_t StringDifference;
// Define a union to hold different types of data
typedef union {
	int64_t intValue;
	uint64_t uintValue;
	double doubleValue;
	String stringValue;
	void *pointerValue;
} StringTemplateValue;
// Key-Value pair to hold template parameters
typedef struct {
	String key;
	StringTemplateValue value;
} StringTemplateParam;

// Interface

// conversion
CanReturnError(void)
	fun_string_from_pointer(void *ptr, OutputString output, size_t output_size);
CanReturnError(void)
	fun_string_from_int(int64_t num, int base, OutputString output,
						size_t output_size);
CanReturnError(void)
	fun_string_from_double(double num, int afterpoint, OutputString output,
						   size_t output_size);

// string validation
CanReturnError(void)
	fun_string_is_valid(String source, StringLength maximumLength);

// find and compare
StringDifference fun_string_compare(String source, String target);
StringPosition fun_string_index_of(String haystack, String needle,
								   StringPosition start);
// in-place operations
StringLength fun_string_length(String source);
void fun_string_trim_in_place(OutputString source);
void fun_string_reverse_in_place(OutputString source);
// out-of-place operations
CanReturnError(void) fun_string_join(String left, String right,
									 OutputString output, size_t output_size);
CanReturnError(void)
	fun_string_copy(String source, OutputString output, size_t output_size);

// substring and slice operations

/**
 * Extract a substring from source string.
 * 
 * @param source REQUIRED - Source string to extract from
 * @param start REQUIRED - Starting index (0-based)
 * @param length REQUIRED - Number of characters to extract
 * @param output REQUIRED - Buffer to store result
 * @param output_size REQUIRED - Size of output buffer in bytes
 * 
 * @return voidResult with error code
 * 
 * Error codes:
 * - ERROR_CODE_NO_ERROR: Success
 * - ERROR_CODE_NULL_POINTER: source or output is NULL
 * - ERROR_CODE_INDEX_OUT_OF_BOUNDS: start >= source length or start+length > source length
 * - ERROR_CODE_BUFFER_TOO_SMALL: output_size < length + 1
 * 
 * Example:
 * char output[32];
 * fun_string_substring("Hello World", 6, 5, output, sizeof(output));
 * // output = "World"
 */
CanReturnError(void)
	fun_string_substring(String source, size_t start, size_t length,
						 OutputString output, size_t output_size);

/**
 * Extract a slice of string from start to end index (Python-style).
 * Supports negative indices (offset from end).
 * 
 * @param source REQUIRED - Source string to extract from
 * @param start REQUIRED - Starting index (negative = from end)
 * @param end REQUIRED - Ending index (exclusive, negative = from end)
 * @param output REQUIRED - Buffer to store result
 * @param output_size REQUIRED - Size of output buffer in bytes
 * 
 * @return voidResult with error code
 * 
 * Error codes:
 * - ERROR_CODE_NO_ERROR: Success
 * - ERROR_CODE_NULL_POINTER: source or output is NULL
 * - ERROR_CODE_INDEX_OUT_OF_BOUNDS: start or end out of valid range
 * - ERROR_CODE_BUFFER_TOO_SMALL: output_size < (end-start) + 1
 * 
 * Notes:
 * - If start >= end after resolving negatives, returns empty string
 * - Negative indices: -1 = last character, -2 = second to last, etc.
 * 
 * Example:
 * char output[32];
 * fun_string_slice("Hello World", 0, 5, output, sizeof(output));
 * // output = "Hello"
 * fun_string_slice("Hello World", -5, -1, output, sizeof(output));
 * // output = "Worl"
 */
CanReturnError(void) fun_string_slice(String source, int64_t start, int64_t end,
									  OutputString output, size_t output_size);

// templating
CanReturnError(void)
	fun_string_template(String template, StringTemplateParam *params,
						size_t paramCount, OutputString output,
						size_t output_size);

#endif // LIBRARY_STRING_H
