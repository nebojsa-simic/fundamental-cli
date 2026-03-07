#include "string/string.h" // contains our declarations and result type definitions

StringDifference fun_string_compare(String source, String target)
{
	if (source == target)
		return 0;
	if (!source)
		return -1;
	if (!target)
		return 1;

	while (*source && *target && *source == *target) {
		source++;
		target++;
	}

	return (StringDifference)(*source - *target);
}

StringPosition fun_string_index_of(String haystack, String needle,
								   StringPosition start)
{
	if (!haystack || !needle || *needle == '\0')
		return -1;

	String original_begin = haystack; // Store original start position
	haystack += start; // Move to search start position

	StringPosition index_of = start;
	while (*haystack) {
		String h_ptr = haystack;
		String n_ptr = needle;

		// Match entire needle
		while (*n_ptr && *h_ptr == *n_ptr) {
			h_ptr++;
			n_ptr++;
		}

		// Found complete match
		if (*n_ptr == '\0') {
			return index_of;
		}

		haystack++;
		index_of++;
	}

	return -1;
}

// In-place operations

StringLength fun_string_length(String source)
{
	if (!source)
		return 0;

	StringLength length = 0;
	while (source[length] != '\0') {
		length++;
	}
	return length;
}

void fun_string_trim_in_place(OutputString source)
{
	if (!source)
		return;

	// Trim leading whitespace
	OutputString start = source;
	while (*start == ' ' || *start == '\t' || *start == '\n' ||
		   *start == '\r') {
		start++;
	}

	if (start != source) {
		OutputString dest = source;
		while (*start) {
			*dest++ = *start++;
		}
		*dest = '\0';
	}

	// Trim trailing whitespace
	OutputString end = source + fun_string_length(source) - 1;
	while (end > source &&
		   (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) {
		end--;
	}
	*(end + 1) = '\0';
}

void fun_string_reverse_in_place(OutputString source)
{
	if (!source)
		return;

	OutputString start = source;
	OutputString end = source + fun_string_length(source) - 1;

	while (start < end) {
		char temp = *start;
		*start = *end;
		*end = temp;
		start++;
		end--;
	}
}

// Out-of-place operations

void fun_string_join(String left, String right, OutputString output)
{
	if (!output)
		return;

	while (left && *left) {
		*output++ = *left++;
	}

	while (right && *right) {
		*output++ = *right++;
	}

	*output = '\0';
}

void fun_string_copy(String source, OutputString output)
{
	if (!source || !output)
		return;

	while (*source) {
		*output++ = *source++;
	}
	*output = '\0';
}
