#include "fundamental/console/console.h"
#include <unistd.h>

ErrorResult platform_console_flush_stdout(const char *data, size_t length)
{
	ssize_t written = write(STDOUT_FILENO, data, length);

	if (written < 0) {
		return fun_error_result(ERROR_CODE_NULL_POINTER, "write() failed");
	}

	return ERROR_RESULT_NO_ERROR;
}

ErrorResult platform_console_flush_stderr(const char *data, size_t length)
{
	ssize_t written = write(STDERR_FILENO, data, length);

	if (written < 0) {
		return fun_error_result(ERROR_CODE_NULL_POINTER, "write() failed");
	}

	return ERROR_RESULT_NO_ERROR;
}
