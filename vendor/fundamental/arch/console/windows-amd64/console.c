#include "console/console.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

ErrorResult platform_console_flush_stdout(const char *data, size_t length) {
  HANDLE hStd = GetStdHandle(STD_OUTPUT_HANDLE);

  if (hStd == INVALID_HANDLE_VALUE) {
    return fun_error_result(ERROR_CODE_NULL_POINTER, "Invalid stdout handle");
  }

  DWORD written = 0;
  if (!WriteFile(hStd, data, (DWORD)length, &written, NULL)) {
    return fun_error_result(ERROR_CODE_NULL_POINTER, "WriteFile failed");
  }

  return ERROR_RESULT_NO_ERROR;
}

ErrorResult platform_console_flush_stderr(const char *data, size_t length) {
  HANDLE hStd = GetStdHandle(STD_ERROR_HANDLE);

  if (hStd == INVALID_HANDLE_VALUE) {
    return fun_error_result(ERROR_CODE_NULL_POINTER, "Invalid stderr handle");
  }

  DWORD written = 0;
  if (!WriteFile(hStd, data, (DWORD)length, &written, NULL)) {
    return fun_error_result(ERROR_CODE_NULL_POINTER, "WriteFile failed");
  }

  return ERROR_RESULT_NO_ERROR;
}
