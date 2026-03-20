#include "fundamental/string/string.h" // contains our declarations and result type definitions

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

voidResult fun_string_join(String left, String right, OutputString output,
						   size_t output_size)
{
	voidResult out;
	if (!output) {
		out.error = fun_error_result(ERROR_CODE_NULL_POINTER, "Null output");
		return out;
	}

	StringLength left_len = left ? fun_string_length(left) : 0;
	StringLength right_len = right ? fun_string_length(right) : 0;
	if (left_len + right_len + 1 > output_size) {
		out.error = fun_error_result(ERROR_CODE_BUFFER_TOO_SMALL,
									 "Output buffer too small");
		return out;
	}

	while (left && *left)
		*output++ = *left++;
	while (right && *right)
		*output++ = *right++;
	*output = '\0';

	out.error = ERROR_RESULT_NO_ERROR;
	return out;
}

voidResult fun_string_copy(String source, OutputString output,
						   size_t output_size)
{
	voidResult out;
	if (!source || !output) {
		out.error = fun_error_result(ERROR_CODE_NULL_POINTER, "Null argument");
		return out;
	}

	StringLength len = fun_string_length(source);
	if (len + 1 > output_size) {
		out.error = fun_error_result(ERROR_CODE_BUFFER_TOO_SMALL,
									 "Output buffer too small");
		return out;
	}

	while (*source)
		*output++ = *source++;
	*output = '\0';

	out.error = ERROR_RESULT_NO_ERROR;
	return out;
}

// Substring and slice operations

voidResult fun_string_substring(String source, size_t start, size_t length,
								OutputString output, size_t output_size)
{
	voidResult out;

	// Task 2.2, 3.1: NULL parameter validation
	if (source == NULL || output == NULL) {
		out.error = fun_error_result(ERROR_CODE_NULL_POINTER,
									 "source or output is NULL");
		return out;
	}

	// Task 2.3, 3.2: Bounds checking
	size_t source_len = fun_string_length(source);
	if (start >= source_len) {
		out.error = fun_error_result(ERROR_CODE_INDEX_OUT_OF_BOUNDS,
									 "start index out of bounds");
		return out;
	}

	if (start + length > source_len) {
		out.error = fun_error_result(ERROR_CODE_INDEX_OUT_OF_BOUNDS,
									 "start + length exceeds source length");
		return out;
	}

	// Task 2.7, 3.3: Buffer size validation
	if (output_size < length + 1) {
		out.error = fun_error_result(ERROR_CODE_BUFFER_TOO_SMALL,
									 "output buffer too small");
		return out;
	}

	// Extract substring
	source += start;
	for (size_t i = 0; i < length; i++) {
		output[i] = source[i];
	}

	// Task 3.4: Null termination
	output[length] = '\0';

	out.error = ERROR_RESULT_NO_ERROR;
	return out;
}

voidResult fun_string_slice(String source, int64_t start, int64_t end,
							OutputString output, size_t output_size)
{
	voidResult out;

	// Task 3.1: NULL parameter validation
	if (source == NULL || output == NULL) {
		out.error = fun_error_result(ERROR_CODE_NULL_POINTER,
									 "source or output is NULL");
		return out;
	}

	size_t source_len = fun_string_length(source);

	// Task 2.5: Negative index resolution
	int64_t resolved_start = start;
	int64_t resolved_end = end;

	if (start < 0) {
		resolved_start = (int64_t)source_len + start;
	}

	if (end < 0) {
		resolved_end = (int64_t)source_len + end;
	}

	// Task 3.2: Bounds checking
	if (resolved_start < 0 || resolved_start > (int64_t)source_len) {
		out.error = fun_error_result(ERROR_CODE_INDEX_OUT_OF_BOUNDS,
									 "start index out of bounds");
		return out;
	}

	if (resolved_end < 0 || resolved_end > (int64_t)source_len) {
		out.error = fun_error_result(ERROR_CODE_INDEX_OUT_OF_BOUNDS,
									 "end index out of bounds");
		return out;
	}

	// Task 2.6: Handle start >= end (return empty string)
	if (resolved_start >= resolved_end) {
		output[0] = '\0';
		out.error = ERROR_RESULT_NO_ERROR;
		return out;
	}

	// Task 2.7, 3.3: Buffer size validation
	size_t slice_length = (size_t)(resolved_end - resolved_start);
	if (output_size < slice_length + 1) {
		out.error = fun_error_result(ERROR_CODE_BUFFER_TOO_SMALL,
									 "output buffer too small");
		return out;
	}

	// Extract slice
	source += resolved_start;
	for (size_t i = 0; i < slice_length; i++) {
		output[i] = source[i];
	}

	// Task 3.4: Null termination
	output[slice_length] = '\0';

	out.error = ERROR_RESULT_NO_ERROR;
	return out;
}
