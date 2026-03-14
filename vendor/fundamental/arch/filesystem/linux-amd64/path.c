// POSIX path utilities implementation

#define _POSIX_C_SOURCE 200809L
#include <unistd.h>
#include <stddef.h>

// Platform-specific path separator for POSIX systems
char fun_platform_path_separator(void)
{
	return '/';
}

// Get current working directory
int fun_platform_get_working_directory(char *output, size_t output_size)
{
	if (output == NULL || output_size == 0) {
		return -1;
	}

	char *result = getcwd(output, output_size);
	if (result == NULL) {
		output[0] = '\0';
		return -1;
	}

	return 0;
}
