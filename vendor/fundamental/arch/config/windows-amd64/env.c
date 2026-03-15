/*
 * Windows AMD64 platform implementation for config module.
 *
 * Provides:
 *   fun_platform_env_lookup     - Read env var using GetEnvironmentVariableA()
 *   fun_platform_get_executable_dir - Get directory of running executable
 *   fun_platform_read_text_file - Read a text file of unknown size
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stddef.h>
#include <stdint.h>

/*
 * Look up an environment variable by its fully-transformed name.
 *
 * @param env_var_name  Null-terminated env var name (e.g., "MYAPP_DATABASE_HOST")
 * @param out_buf       Buffer to receive the value
 * @param buf_size      Size of out_buf in bytes
 * @return 0 on success, -1 if not found or buf_size too small
 */
int fun_platform_env_lookup(const char *env_var_name, char *out_buf,
							size_t buf_size)
{
	if (!env_var_name || !out_buf || buf_size == 0)
		return -1;

	DWORD result =
		GetEnvironmentVariableA(env_var_name, out_buf, (DWORD)buf_size);
	if (result == 0 || result >= (DWORD)buf_size)
		return -1;

	return 0;
}

/*
 * Get the directory containing the running executable.
 *
 * Uses GetModuleFileNameA() to find the executable path.
 *
 * @param out_dir   Buffer to receive the directory path (null-terminated)
 * @param buf_size  Size of out_dir in bytes
 * @return 0 on success, -1 on error
 */
int fun_platform_get_executable_dir(char *out_dir, size_t buf_size)
{
	if (!out_dir || buf_size < 2)
		return -1;

	DWORD len = GetModuleFileNameA(NULL, out_dir, (DWORD)(buf_size - 1));
	if (len == 0 || len >= (DWORD)(buf_size - 1))
		return -1;

	out_dir[len] = '\0';

	/* Strip the filename, keep the directory */
	int last_sep = -1;
	for (int i = 0; i < (int)len; i++) {
		if (out_dir[i] == '\\' || out_dir[i] == '/')
			last_sep = i;
	}

	if (last_sep < 0) {
		out_dir[0] = '.';
		out_dir[1] = '\0';
	} else {
		out_dir[last_sep] = '\0';
	}

	return 0;
}

/*
 * Read an entire text file into a buffer.
 *
 * @param path           Path to the file
 * @param buffer         Output buffer
 * @param max_size       Maximum bytes to read (buffer must be at least max_size+1)
 * @param out_bytes_read Set to actual bytes read on success
 * @return 0 on success, -1 if file not found, -2 on other error
 */
int fun_platform_read_text_file(const char *path, char *buffer, size_t max_size,
								size_t *out_bytes_read)
{
	if (!path || !buffer || !out_bytes_read || max_size == 0)
		return -2;

	HANDLE hFile = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL,
							   OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		DWORD err = GetLastError();
		return (err == ERROR_FILE_NOT_FOUND || err == ERROR_PATH_NOT_FOUND) ?
				   -1 :
				   -2;
	}

	DWORD bytes_read = 0;
	BOOL ok = ReadFile(hFile, buffer, (DWORD)max_size, &bytes_read, NULL);
	CloseHandle(hFile);

	if (!ok)
		return -2;

	buffer[bytes_read] = '\0';
	*out_bytes_read = (size_t)bytes_read;
	return 0;
}
