#include <stddef.h>
#include <stdint.h>

/* Saved environment from startup */
extern const char **fun_arch_envp;

/* ---- Syscall numbers ---- */
#define SYS_read     0
#define SYS_open     2
#define SYS_close    3
#define SYS_readlink 89

#define O_RDONLY 0

/* ---- Syscall helpers ---- */
static inline long syscall1(long n, long a1)
{
	long ret;
	__asm__ __volatile__("syscall"
						 : "=a"(ret)
						 : "a"(n), "D"(a1)
						 : "rcx", "r11", "memory");
	return ret;
}

static inline long syscall2(long n, long a1, long a2)
{
	long ret;
	__asm__ __volatile__("syscall"
						 : "=a"(ret)
						 : "a"(n), "D"(a1), "S"(a2)
						 : "rcx", "r11", "memory");
	return ret;
}

static inline long syscall3(long n, long a1, long a2, long a3)
{
	long ret;
	__asm__ __volatile__("syscall"
						 : "=a"(ret)
						 : "a"(n), "D"(a1), "S"(a2), "d"(a3)
						 : "rcx", "r11", "memory");
	return ret;
}

/*
 * Look up an environment variable by name.
 * Searches fun_arch_envp directly — no getenv().
 */
int fun_platform_env_lookup(const char *env_var_name, char *out_buf,
							size_t buf_size)
{
	if (!env_var_name || !out_buf || buf_size == 0)
		return -1;
	if (!fun_arch_envp)
		return -1;

	size_t name_len = 0;
	while (env_var_name[name_len])
		name_len++;

	for (int i = 0; fun_arch_envp[i] != NULL; i++) {
		const char *e = fun_arch_envp[i];
		size_t j;
		for (j = 0; j < name_len; j++) {
			if (e[j] != env_var_name[j])
				break;
		}
		if (j == name_len && e[j] == '=') {
			const char *val = e + j + 1;
			size_t k = 0;
			while (val[k] && k < buf_size - 1) {
				out_buf[k] = val[k];
				k++;
			}
			out_buf[k] = '\0';
			return 0;
		}
	}
	return -1;
}

/*
 * Get directory of the running executable via /proc/self/exe.
 */
int fun_platform_get_executable_dir(char *out_dir, size_t buf_size)
{
	if (!out_dir || buf_size < 2)
		return -1;

	long len = syscall3(SYS_readlink, (long)"/proc/self/exe",
						(long)out_dir, (long)(buf_size - 1));
	if (len < 0)
		return -1;

	out_dir[len] = '\0';

	long last_sep = -1;
	for (long i = 0; i < len; i++) {
		if (out_dir[i] == '/')
			last_sep = i;
	}

	if (last_sep < 0) {
		out_dir[0] = '.';
		out_dir[1] = '\0';
	} else if (last_sep == 0) {
		out_dir[1] = '\0';
	} else {
		out_dir[last_sep] = '\0';
	}

	return 0;
}

/*
 * Read an entire text file into a buffer.
 */
int fun_platform_read_text_file(const char *path, char *buffer, size_t max_size,
								size_t *out_bytes_read)
{
	if (!path || !buffer || !out_bytes_read || max_size == 0)
		return -2;

	long fd = syscall2(SYS_open, (long)path, O_RDONLY);
	if (fd < 0)
		return -1;

	size_t total = 0;
	while (total < max_size) {
		long n = syscall3(SYS_read, fd, (long)(buffer + total),
						  (long)(max_size - total));
		if (n < 0) {
			syscall1(SYS_close, fd);
			return -2;
		}
		if (n == 0)
			break;
		total += (size_t)n;
	}

	syscall1(SYS_close, fd);
	buffer[total] = '\0';
	*out_bytes_read = total;
	return 0;
}
