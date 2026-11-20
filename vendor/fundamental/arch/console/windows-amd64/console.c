#include "fundamental/console/console.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

/* WriteFile to a console handle interprets bytes in the console's output
 * codepage (default CP437/CP850). The library emits UTF-8, so switch the
 * console to UTF-8 once, but only when stdout is an actual console (redirected
 * /piped output already passes raw bytes through unchanged). */
static void ensure_utf8_console_output(void)
{
	static int done = 0;
	if (done)
		return;
	done = 1;

	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hOut != INVALID_HANDLE_VALUE &&
		GetFileType(hOut) == FILE_TYPE_CHAR)
		SetConsoleOutputCP(CP_UTF8);
}

ErrorResult platform_console_flush_stdout(const char *data, size_t length)
{
	ensure_utf8_console_output();

	HANDLE hStd = GetStdHandle(STD_OUTPUT_HANDLE);

	if (hStd == INVALID_HANDLE_VALUE) {
		return fun_error_result(ERROR_CODE_NULL_POINTER,
								"Invalid stdout handle");
	}

	DWORD written = 0;
	if (!WriteFile(hStd, data, (DWORD)length, &written, NULL)) {
		return fun_error_result(ERROR_CODE_NULL_POINTER, "WriteFile failed");
	}

	return ERROR_RESULT_NO_ERROR;
}

ErrorResult platform_console_flush_stderr(const char *data, size_t length)
{
	HANDLE hStd = GetStdHandle(STD_ERROR_HANDLE);

	if (hStd == INVALID_HANDLE_VALUE) {
		return fun_error_result(ERROR_CODE_NULL_POINTER,
								"Invalid stderr handle");
	}

	DWORD written = 0;
	if (!WriteFile(hStd, data, (DWORD)length, &written, NULL)) {
		return fun_error_result(ERROR_CODE_NULL_POINTER, "WriteFile failed");
	}

	return ERROR_RESULT_NO_ERROR;
}
