#define _POSIX_C_SOURCE 200809L
#include "fundamental/process/process.h"

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <unistd.h>

/* ------------------------------------------------------------------
 * Internal handle layout stored in ProcessResult->_handle.
 * Uses mmap/munmap-free approach: pack two ints (fds) + pid into a
 * pointer-sized value. On 64-bit Linux pid_t and int fit in 64 bits.
 *
 * Layout: _handle cast to LinuxProcHandle*
 * We use a small malloc-equivalent via a static inline using mmap.
 * ------------------------------------------------------------------ */
#include <sys/mman.h>

typedef struct {
	pid_t pid;
	int stdout_fd;
	int stderr_fd;
} LinuxProcHandle;

static LinuxProcHandle *alloc_handle(void)
{
	void *p = mmap(NULL, sizeof(LinuxProcHandle), PROT_READ | PROT_WRITE,
				   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (p == MAP_FAILED)
		return NULL;
	return (LinuxProcHandle *)p;
}

static void free_handle(LinuxProcHandle *h)
{
	if (h != NULL)
		munmap(h, sizeof(LinuxProcHandle));
}

static int executable_exists(const char *path)
{
	return access(path, X_OK) == 0;
}

static void drain_fd(int fd, char *buf, size_t capacity, size_t *length)
{
	if (fd < 0 || buf == NULL || capacity == 0)
		return;

	while (*length < capacity) {
		int avail = 0;
		if (ioctl(fd, FIONREAD, &avail) < 0 || avail == 0)
			break;
		size_t to_read = (size_t)avail;
		if (to_read > capacity - *length)
			to_read = capacity - *length;
		ssize_t n = read(fd, buf + *length, to_read);
		if (n <= 0)
			break;
		*length += (size_t)n;
	}
}

static AsyncStatus linux_process_poll(AsyncResult *result)
{
	ProcessResult *out = (ProcessResult *)result->state;
	if (out == NULL || out->_handle == NULL)
		return ASYNC_ERROR;

	LinuxProcHandle *h = (LinuxProcHandle *)out->_handle;

	/* Drain pipes while process may still be running */
	drain_fd(h->stdout_fd, out->stdout_data, out->stdout_capacity,
			 &out->stdout_length);
	drain_fd(h->stderr_fd, out->stderr_data, out->stderr_capacity,
			 &out->stderr_length);

	int status = 0;
	pid_t ret = waitpid(h->pid, &status, WNOHANG);

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
		close(h->stdout_fd);
		h->stdout_fd = -1;
	}
	if (h->stderr_fd >= 0) {
		close(h->stderr_fd);
		h->stderr_fd = -1;
	}

	result->status = ASYNC_COMPLETED;
	return ASYNC_COMPLETED;
}

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

	if (!executable_exists(executable)) {
		result.status = ASYNC_ERROR;
		result.error = fun_error_result(ERROR_CODE_PROCESS_NOT_FOUND,
										"Executable not found");
		return result;
	}

	int stdout_pipe[2], stderr_pipe[2];

	if (pipe(stdout_pipe) < 0) {
		result.status = ASYNC_ERROR;
		result.error = fun_error_result(ERROR_CODE_PROCESS_SPAWN_FAILED,
										"Failed to create stdout pipe");
		return result;
	}
	if (pipe(stderr_pipe) < 0) {
		close(stdout_pipe[0]);
		close(stdout_pipe[1]);
		result.status = ASYNC_ERROR;
		result.error = fun_error_result(ERROR_CODE_PROCESS_SPAWN_FAILED,
										"Failed to create stderr pipe");
		return result;
	}

	pid_t pid = fork();

	if (pid < 0) {
		close(stdout_pipe[0]);
		close(stdout_pipe[1]);
		close(stderr_pipe[0]);
		close(stderr_pipe[1]);
		result.status = ASYNC_ERROR;
		result.error =
			fun_error_result(ERROR_CODE_PROCESS_SPAWN_FAILED, "Fork failed");
		return result;
	}

	if (pid == 0) {
		/* Child */
		close(stdout_pipe[0]);
		close(stderr_pipe[0]);
		dup2(stdout_pipe[1], STDOUT_FILENO);
		dup2(stderr_pipe[1], STDERR_FILENO);
		close(stdout_pipe[1]);
		close(stderr_pipe[1]);

		if (args != NULL) {
			execvp(executable, (char *const *)args);
		} else {
			const char *argv[] = { executable, NULL };
			execvp(executable, (char *const *)argv);
		}
		_exit(127);
	}

	/* Parent */
	close(stdout_pipe[1]);
	close(stderr_pipe[1]);

	fcntl(stdout_pipe[0], F_SETFL,
		  fcntl(stdout_pipe[0], F_GETFL, 0) | O_NONBLOCK);
	fcntl(stderr_pipe[0], F_SETFL,
		  fcntl(stderr_pipe[0], F_GETFL, 0) | O_NONBLOCK);

	LinuxProcHandle *h = alloc_handle();
	if (h == NULL) {
		kill(pid, SIGKILL);
		waitpid(pid, NULL, 0);
		close(stdout_pipe[0]);
		close(stderr_pipe[0]);
		result.status = ASYNC_ERROR;
		result.error = fun_error_result(ERROR_CODE_PROCESS_SPAWN_FAILED,
										"Failed to allocate process handle");
		return result;
	}

	h->pid = pid;
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
		if (kill(h->pid, SIGKILL) < 0 && errno != ESRCH) {
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
		close(h->stdout_fd);
		h->stdout_fd = -1;
	}
	if (h->stderr_fd >= 0) {
		close(h->stderr_fd);
		h->stderr_fd = -1;
	}
	if (h->pid > 0) {
		int status;
		waitpid(h->pid, &status, WNOHANG);
		h->pid = 0;
	}

	free_handle(h);
	out->_handle = NULL;
	return r;
}
