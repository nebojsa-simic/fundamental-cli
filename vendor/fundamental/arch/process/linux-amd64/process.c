#include <stddef.h>

#include "fundamental/process/process.h"
#include "fundamental/string/string.h"

/* ---- Syscall numbers ---- */
#define SYS_read 0
#define SYS_close 3
#define SYS_mmap 9
#define SYS_munmap 11
#define SYS_ioctl 16
#define SYS_access 21
#define SYS_pipe 22
#define SYS_dup2 33
#define SYS_fork 57
#define SYS_execve 59
#define SYS_exit 60
#define SYS_wait4 61
#define SYS_kill 62
#define SYS_fcntl 72

/* ---- Constants ---- */
#define PROT_READ 0x1
#define PROT_WRITE 0x2
#define MAP_PRIVATE 0x2
#define MAP_ANONYMOUS 0x20
#define MAP_FAILED ((void *)-1)
#define X_OK 1
#define SIGKILL 9
#define WNOHANG 1
#define O_NONBLOCK 2048
#define F_GETFL 3
#define F_SETFL 4
#define FIONREAD 0x541BL
#define WIFEXITED(s) (((s) & 0x7f) == 0)
#define WEXITSTATUS(s) (((s) >> 8) & 0xff)
#define ESRCH 3

/* ---- Saved environment from startup ---- */
extern const char **fun_arch_envp;

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

static inline long syscall4(long n, long a1, long a2, long a3, long a4)
{
	long ret;
	register long r10 __asm__("r10") = a4;
	__asm__ __volatile__("syscall"
						 : "=a"(ret)
						 : "a"(n), "D"(a1), "S"(a2), "d"(a3), "r"(r10)
						 : "rcx", "r11", "memory");
	return ret;
}

static inline long syscall6(long n, long a1, long a2, long a3, long a4, long a5,
							long a6)
{
	long ret;
	register long r10 __asm__("r10") = a4;
	register long r8 __asm__("r8") = a5;
	register long r9 __asm__("r9") = a6;
	__asm__ __volatile__("syscall"
						 : "=a"(ret)
						 : "a"(n), "D"(a1), "S"(a2), "d"(a3), "r"(r10), "r"(r8),
						   "r"(r9)
						 : "rcx", "r11", "memory");
	return ret;
}

static inline void *sys_mmap(size_t size)
{
	return (void *)syscall6(SYS_mmap, 0, (long)size, PROT_READ | PROT_WRITE,
							MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
}

static inline void sys_munmap(void *p, size_t size)
{
	syscall2(SYS_munmap, (long)p, (long)size);
}

/* ---- Process handle ---- */
typedef struct {
	int pid;
	int stdout_fd;
	int stderr_fd;
} LinuxProcHandle;

static LinuxProcHandle *alloc_handle(void)
{
	void *p = sys_mmap(sizeof(LinuxProcHandle));
	if (p == MAP_FAILED)
		return NULL;
	return (LinuxProcHandle *)p;
}

static void free_handle(LinuxProcHandle *h)
{
	if (h != NULL)
		sys_munmap(h, sizeof(LinuxProcHandle));
}

/* ---- Find executable path (searches PATH env for non-absolute names) ---- */
static int find_executable(const char *name, char *out, size_t out_size)
{
	/* Check if name contains '/' — use directly */
	const char *p = name;
	while (*p && *p != '/')
		p++;
	if (*p == '/') {
		fun_string_copy((String)name, out, out_size);
		return syscall2(SYS_access, (long)out, X_OK) == 0 ? 1 : 0;
	}

	/* Search PATH */
	const char *path_val = NULL;
	if (fun_arch_envp != NULL) {
		for (int i = 0; fun_arch_envp[i] != NULL; i++) {
			const char *e = fun_arch_envp[i];
			if (e[0] == 'P' && e[1] == 'A' && e[2] == 'T' && e[3] == 'H' &&
				e[4] == '=') {
				path_val = e + 5;
				break;
			}
		}
	}
	if (path_val == NULL)
		return 0;

	StringLength name_len = fun_string_length(name);
	const char *pp = path_val;
	while (*pp) {
		const char *end = pp;
		while (*end && *end != ':')
			end++;
		size_t dir_len = (size_t)(end - pp);

		if (dir_len + 1 + name_len + 1 <= out_size) {
			size_t i = 0;
			for (size_t j = 0; j < dir_len; j++)
				out[i++] = pp[j];
			out[i++] = '/';
			fun_string_copy(name, out + i, out_size - i);

			if (syscall2(SYS_access, (long)out, X_OK) == 0)
				return 1;
		}

		pp = (*end == ':') ? end + 1 : end;
		if (*pp == '\0')
			break;
	}
	return 0;
}

/* ---- drain_fd: non-blocking read from fd into buffer ---- */
static void drain_fd(int fd, char *buf, size_t capacity, size_t *length)
{
	if (fd < 0 || buf == NULL || capacity == 0)
		return;

	while (*length < capacity) {
		int avail = 0;
		if (syscall3(SYS_ioctl, (long)fd, FIONREAD, (long)&avail) < 0 ||
			avail == 0)
			break;
		size_t to_read = (size_t)avail;
		if (to_read > capacity - *length)
			to_read = capacity - *length;
		long n =
			syscall3(SYS_read, (long)fd, (long)(buf + *length), (long)to_read);
		if (n <= 0)
			break;
		*length += (size_t)n;
	}
}

/* ---- Poll callback ---- */
static AsyncStatus linux_process_poll(AsyncResult *result)
{
	ProcessResult *out = (ProcessResult *)result->state;
	if (out == NULL || out->_handle == NULL)
		return ASYNC_ERROR;

	LinuxProcHandle *h = (LinuxProcHandle *)out->_handle;

	drain_fd(h->stdout_fd, out->stdout_data, out->stdout_capacity,
			 &out->stdout_length);
	drain_fd(h->stderr_fd, out->stderr_data, out->stderr_capacity,
			 &out->stderr_length);

	int status = 0;
	long ret = syscall4(SYS_wait4, (long)h->pid, (long)&status, WNOHANG, 0);

	if (ret < 0)
		return ASYNC_ERROR;
	if (ret == 0)
		return ASYNC_PENDING;

	/* Process exited — final drain */
	drain_fd(h->stdout_fd, out->stdout_data, out->stdout_capacity,
			 &out->stdout_length);
	drain_fd(h->stderr_fd, out->stderr_data, out->stderr_capacity,
			 &out->stderr_length);

	if (WIFEXITED(status))
		out->exit_code = WEXITSTATUS(status);
	else
		out->exit_code = -1;

	if (h->stdout_fd >= 0) {
		syscall1(SYS_close, (long)h->stdout_fd);
		h->stdout_fd = -1;
	}
	if (h->stderr_fd >= 0) {
		syscall1(SYS_close, (long)h->stderr_fd);
		h->stderr_fd = -1;
	}

	result->status = ASYNC_COMPLETED;
	return ASYNC_COMPLETED;
}

/* ---- Spawn ---- */
AsyncResult fun_process_arch_spawn(const char *executable, const char **args,
								   const ProcessSpawnOptions *options,
								   ProcessResult *out)
{
	(void)options;

	AsyncResult result;
	result.poll = linux_process_poll;
	result.state = out;
	result.status = ASYNC_PENDING;
	result.error = ERROR_RESULT_NO_ERROR;

	out->stdout_length = 0;
	out->stderr_length = 0;
	out->exit_code = 0;

	char exec_path[512];
	if (!find_executable(executable, exec_path, sizeof(exec_path))) {
		result.status = ASYNC_ERROR;
		result.error = fun_error_result(ERROR_CODE_PROCESS_NOT_FOUND,
										"Executable not found");
		return result;
	}

	int stdout_pipe[2], stderr_pipe[2];

	if (syscall1(SYS_pipe, (long)stdout_pipe) < 0) {
		result.status = ASYNC_ERROR;
		result.error = fun_error_result(ERROR_CODE_PROCESS_SPAWN_FAILED,
										"Failed to create stdout pipe");
		return result;
	}
	if (syscall1(SYS_pipe, (long)stderr_pipe) < 0) {
		syscall1(SYS_close, (long)stdout_pipe[0]);
		syscall1(SYS_close, (long)stdout_pipe[1]);
		result.status = ASYNC_ERROR;
		result.error = fun_error_result(ERROR_CODE_PROCESS_SPAWN_FAILED,
										"Failed to create stderr pipe");
		return result;
	}

	long pid = syscall1(SYS_fork, 0);

	if (pid < 0) {
		syscall1(SYS_close, (long)stdout_pipe[0]);
		syscall1(SYS_close, (long)stdout_pipe[1]);
		syscall1(SYS_close, (long)stderr_pipe[0]);
		syscall1(SYS_close, (long)stderr_pipe[1]);
		result.status = ASYNC_ERROR;
		result.error =
			fun_error_result(ERROR_CODE_PROCESS_SPAWN_FAILED, "Fork failed");
		return result;
	}

	if (pid == 0) {
		/* Child */
		syscall1(SYS_close, (long)stdout_pipe[0]);
		syscall1(SYS_close, (long)stderr_pipe[0]);
		syscall2(SYS_dup2, (long)stdout_pipe[1], 1);
		syscall2(SYS_dup2, (long)stderr_pipe[1], 2);
		syscall1(SYS_close, (long)stdout_pipe[1]);
		syscall1(SYS_close, (long)stderr_pipe[1]);

		const char *empty_env[] = { NULL };
		const char **envp = fun_arch_envp ? fun_arch_envp : empty_env;

		if (args != NULL) {
			syscall3(SYS_execve, (long)exec_path, (long)args, (long)envp);
		} else {
			const char *argv[] = { exec_path, NULL };
			syscall3(SYS_execve, (long)exec_path, (long)argv, (long)envp);
		}
		syscall1(SYS_exit, 127);
		/* unreachable */
		while (1) {
		}
	}

	/* Parent */
	syscall1(SYS_close, (long)stdout_pipe[1]);
	syscall1(SYS_close, (long)stderr_pipe[1]);

	long flags;
	flags = syscall3(SYS_fcntl, (long)stdout_pipe[0], F_GETFL, 0);
	syscall3(SYS_fcntl, (long)stdout_pipe[0], F_SETFL, flags | O_NONBLOCK);
	flags = syscall3(SYS_fcntl, (long)stderr_pipe[0], F_GETFL, 0);
	syscall3(SYS_fcntl, (long)stderr_pipe[0], F_SETFL, flags | O_NONBLOCK);

	LinuxProcHandle *h = alloc_handle();
	if (h == NULL) {
		syscall2(SYS_kill, (long)pid, SIGKILL);
		syscall4(SYS_wait4, pid, 0, 0, 0);
		syscall1(SYS_close, (long)stdout_pipe[0]);
		syscall1(SYS_close, (long)stderr_pipe[0]);
		result.status = ASYNC_ERROR;
		result.error = fun_error_result(ERROR_CODE_PROCESS_SPAWN_FAILED,
										"Failed to allocate process handle");
		return result;
	}

	h->pid = (int)pid;
	h->stdout_fd = stdout_pipe[0];
	h->stderr_fd = stderr_pipe[0];
	out->_handle = h;

	return result;
}

voidResult fun_process_arch_terminate(ProcessResult *out)
{
	voidResult r;
	r.error = ERROR_RESULT_NO_ERROR;

	if (out->_handle == NULL)
		return r;

	LinuxProcHandle *h = (LinuxProcHandle *)out->_handle;
	if (h->pid > 0) {
		long ret = syscall2(SYS_kill, (long)h->pid, SIGKILL);
		if (ret < 0 && -ret != ESRCH) {
			r.error = fun_error_result(ERROR_CODE_PROCESS_TERMINATE_FAILED,
									   "Failed to terminate process");
		}
	}
	return r;
}

voidResult fun_process_arch_free(ProcessResult *out)
{
	voidResult r;
	r.error = ERROR_RESULT_NO_ERROR;

	if (out->_handle == NULL)
		return r;

	LinuxProcHandle *h = (LinuxProcHandle *)out->_handle;

	if (h->stdout_fd >= 0) {
		syscall1(SYS_close, (long)h->stdout_fd);
		h->stdout_fd = -1;
	}
	if (h->stderr_fd >= 0) {
		syscall1(SYS_close, (long)h->stderr_fd);
		h->stderr_fd = -1;
	}
	if (h->pid > 0) {
		int status;
		syscall4(SYS_wait4, (long)h->pid, (long)&status, WNOHANG, 0);
		h->pid = 0;
	}

	free_handle(h);
	out->_handle = NULL;
	return r;
}
