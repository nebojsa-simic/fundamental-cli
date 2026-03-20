#include "fundamental/console/console.h"

#define CONSOLE_BUFFER_SIZE 512

// Static buffer for stdout output
static char console_buffer[CONSOLE_BUFFER_SIZE];
static size_t console_buffer_pos = 0;

// Platform-specific flush function (implemented in arch/console/*/console.c)
extern ErrorResult platform_console_flush_stdout(const char *data,
												 size_t length);
extern ErrorResult platform_console_flush_stderr(const char *data,
												 size_t length);

// Internal buffer flush function
static ErrorResult flush_buffer(void)
{
	if (console_buffer_pos > 0) {
		ErrorResult result =
			platform_console_flush_stdout(console_buffer, console_buffer_pos);
		if (fun_error_is_error(result)) {
			return result;
		}
		console_buffer_pos = 0;
	}
	return ERROR_RESULT_NO_ERROR;
}

ErrorResult fun_console_write(String string)
{
	if (string == NULL) {
		return ERROR_RESULT_NULL_POINTER;
	}

	size_t len = fun_string_length(string);

	// If string would overflow buffer, flush first
	if (console_buffer_pos + len > CONSOLE_BUFFER_SIZE) {
		// Flush existing buffer
		ErrorResult result = flush_buffer();
		if (fun_error_is_error(result)) {
			return result;
		}

		// If string still doesn't fit, write directly
		if (len > CONSOLE_BUFFER_SIZE) {
			return platform_console_flush_stdout(string, len);
		}
	}

	// Copy string to buffer
	for (size_t i = 0; i < len; i++) {
		console_buffer[console_buffer_pos++] = string[i];
	}

	return ERROR_RESULT_NO_ERROR;
}

ErrorResult fun_console_write_line(String string)
{
	if (string == NULL) {
		return ERROR_RESULT_NULL_POINTER;
	}

	// Write the string
	ErrorResult result = fun_console_write(string);
	if (fun_error_is_error(result)) {
		return result;
	}

	// Add newline character
	if (console_buffer_pos < CONSOLE_BUFFER_SIZE) {
		console_buffer[console_buffer_pos++] = '\n';
	}

	// Flush on newline
	return flush_buffer();
}

ErrorResult fun_console_error_line(String string)
{
	if (string == NULL) {
		return ERROR_RESULT_NULL_POINTER;
	}

	size_t len = fun_string_length(string);

	// Write directly to stderr (unbuffered)
	ErrorResult result = platform_console_flush_stderr(string, len);
	if (fun_error_is_error(result)) {
		return result;
	}

	// Write newline to stderr
	return platform_console_flush_stderr("\n", 1);
}

ErrorResult fun_console_flush(void)
{
	return flush_buffer();
}
