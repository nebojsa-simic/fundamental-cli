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
void fun_string_from_pointer(void *ptr, OutputString output);
void fun_string_from_int(int64_t num, int base, OutputString output);
void fun_string_from_double(double num, int afterpoint, OutputString output);

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
void fun_string_join(String left, String right, OutputString output);
void fun_string_copy(String source, OutputString output);

// templating
void fun_string_template(String template, StringTemplateParam *params,
						 size_t paramCount, OutputString output);

#endif // LIBRARY_STRING_H
