#include <stddef.h>
#include <stdint.h>

/* Saved environment from startup - NULL by default for tests */
static const char **fun_arch_envp_local = NULL;

/* Getter/setter for environment pointer */
const char **fun_arch_get_envp(void)
{
	return fun_arch_envp_local;
}

void fun_arch_set_envp(const char **envp)
{
	fun_arch_envp_local = envp;
}

/* Forward declaration */
int fun_platform_read_text_file(const char *path, char *buffer, size_t max_size,
								size_t *out_bytes_read);

/* __environ is set by libc and updated by setenv() - weak reference */
extern char **__environ __attribute__((weak));

/* ---- Syscall numbers ---- */
#define SYS_read 0
#define SYS_open 2
#define SYS_close 3
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
 * Falls back to /proc/self/environ if fun_arch_envp is NULL.
 */
static int lookup_in_env_buffer(const char *env_var_name, const char *env_buf,
								size_t env_size, char *out_buf, size_t buf_size)
{
	size_t name_len = 0;
	while (env_var_name[name_len])
		name_len++;

	size_t i = 0;
	while (i < env_size) {
		const char *e = env_buf + i;
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
		while (i < env_size && env_buf[i] != '\0')
			i++;
		i++;
	}
	return -1;
}

int fun_platform_env_lookup(const char *env_var_name, char *out_buf,
							size_t buf_size)
{
	const char **envp = fun_arch_get_envp();

	if (!env_var_name || !out_buf || buf_size == 0)
		return -1;

	if (envp) {
		size_t name_len = 0;
		while (env_var_name[name_len])
			name_len++;

		for (int i = 0; envp[i] != NULL; i++) {
			const char *e = envp[i];
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

	char env_buf[65536];
	size_t bytes_read = 0;
	int ret = fun_platform_read_text_file("/proc/self/environ", env_buf,
										  sizeof(env_buf), &bytes_read);
	if (ret != 0)
		return -1;

	return lookup_in_env_buffer(env_var_name, env_buf, bytes_read, out_buf,
								buf_size);
}

/*
 * Get directory of the running executable via /proc/self/exe.
 */
int fun_platform_get_executable_dir(char *out_dir, size_t buf_size)
{
	if (!out_dir || buf_size < 2)
		return -1;

	long len = syscall3(SYS_readlink, (long)"/proc/self/exe", (long)out_dir,
						(long)(buf_size - 1));
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
