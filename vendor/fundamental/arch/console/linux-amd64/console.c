#include "fundamental/console/console.h"

static inline long sys_write(int fd, const void *buf, unsigned long count)
{
	long ret;
	__asm__ __volatile__("syscall"
						 : "=a"(ret)
						 : "0"(1L), "D"((long)fd), "S"(buf), "d"(count)
						 : "rcx", "r11", "memory");
	return ret;
}

ErrorResult platform_console_flush_stdout(const char *data, size_t length)
{
	if (sys_write(1, data, length) < 0)
		return fun_error_result(ERROR_CODE_NULL_POINTER, "write() failed");
	return ERROR_RESULT_NO_ERROR;
}

ErrorResult platform_console_flush_stderr(const char *data, size_t length)
{
	if (sys_write(2, data, length) < 0)
		return fun_error_result(ERROR_CODE_NULL_POINTER, "write() failed");
	return ERROR_RESULT_NO_ERROR;
}
