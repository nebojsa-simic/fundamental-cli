#include <stddef.h>

char fun_platform_path_separator(void)
{
	return '/';
}

int fun_platform_get_working_directory(char *output, size_t output_size)
{
	if (output == (void *)0 || output_size == 0)
		return -1;

	long ret;
	__asm__ __volatile__(
		"syscall"
		: "=a"(ret)
		: "0"(79L), "D"(output), "S"(output_size)
		: "rcx", "r11", "memory");

	if (ret < 0) {
		output[0] = '\0';
		return -1;
	}
	return 0;
}
