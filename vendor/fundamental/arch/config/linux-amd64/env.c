/*
 * Linux AMD64 platform implementation for config module.
 *
 * Provides:
 *   fun_platform_env_lookup     - Read env var by name using getenv()
 *   fun_platform_get_executable_dir - Get directory of the running executable
 *   fun_platform_read_text_file - Read a text file of unknown size
 */

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stddef.h>
#include <stdint.h>

/*
 * Look up an environment variable by its fully-transformed name.
 * (Caller is responsible for uppercasing and dot→underscore transformation.)
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

	const char *val = getenv(env_var_name);
	if (!val)
		return -1;

	/* Copy value into out_buf */
	size_t i = 0;
	while (val[i] && i < buf_size - 1) {
		out_buf[i] = val[i];
		i++;
	}
	out_buf[i] = '\0';

	return 0;
}

/*
 * Get the directory containing the running executable.
 *
 * Uses /proc/self/exe via readlink().
 *
 * @param out_dir   Buffer to receive the directory path (null-terminated)
 * @param buf_size  Size of out_dir in bytes
 * @return 0 on success, -1 on error
 */
int fun_platform_get_executable_dir(char *out_dir, size_t buf_size)
{
	if (!out_dir || buf_size < 2)
		return -1;

	long len = (long)readlink("/proc/self/exe", out_dir, buf_size - 1);
	if (len < 0)
		return -1;

	out_dir[len] = '\0';

	/* Strip the filename, keep the directory */
	long last_sep = -1;
	for (long i = 0; i < len; i++) {
		if (out_dir[i] == '/')
			last_sep = i;
	}

	if (last_sep < 0) {
		/* No separator found - use current directory */
		out_dir[0] = '.';
		out_dir[1] = '\0';
	} else if (last_sep == 0) {
		/* Executable is in root */
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

	int fd = open(path, O_RDONLY);
	if (fd < 0)
		return -1; /* File not found or inaccessible */

	size_t total = 0;
	long n;
	while (total < max_size) {
		n = (long)read(fd, buffer + total, max_size - total);
		if (n < 0) {
			close(fd);
			return -2;
		}
		if (n == 0)
			break; /* EOF */
		total += (size_t)n;
	}

	close(fd);
	buffer[total] = '\0';
	*out_bytes_read = total;
	return 0;
}
