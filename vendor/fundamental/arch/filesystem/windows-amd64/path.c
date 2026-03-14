// Windows path utilities implementation

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// Platform-specific path separator for Windows
char fun_platform_path_separator(void)
{
	return '\\';
}

// Get current working directory
int fun_platform_get_working_directory(char *output, size_t output_size)
{
	if (output == NULL || output_size == 0) {
		return -1;
	}

	DWORD result = GetCurrentDirectoryA((DWORD)output_size, output);
	if (result == 0 || result >= (DWORD)output_size) {
		output[0] = '\0';
		return -1;
	}

	return 0;
}
