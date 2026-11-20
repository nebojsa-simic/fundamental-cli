#include "fundamental/file/file.h"
#include "fundamental/memory/memory.h"
#include "fundamental/error/error.h"
#include "fundamental/async/async.h"
#include "syscall_nums.h"

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct {
	String file_path;
	FileChangeCallback callback;
	int inotify_fd;
	bool inotify_fd_valid;
	int watch_fd;
	bool watch_fd_valid;
	bool monitoring_active;
} FileNotificationState;

struct pollfd {
	int fd;
	short events;
	short revents;
};

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

struct inotify_event {
	int wd;
	uint32_t mask;
	uint32_t cookie;
	uint32_t len;
	char name[];
};

static void strlen_local(const char *s, size_t *len)
{
	*len = 0;
	while (s[*len] != '\0') {
		(*len)++;
	}
}

static void strcpy_local(char *dest, const char *src)
{
	size_t i = 0;
	while (src[i] != '\0') {
		dest[i] = src[i];
		i++;
	}
	dest[i] = '\0';
}

static void get_parent_dir(const char *path, char *dir_buf)
{
	strcpy_local(dir_buf, path);
	size_t len;
	strlen_local(dir_buf, &len);

	while (len > 0 && dir_buf[len - 1] != '/') {
		len--;
	}
	if (len > 0) {
		dir_buf[len - 1] = '\0';
	} else {
		dir_buf[0] = '.';
		dir_buf[1] = '\0';
	}
}

static void get_filename(const char *path, const char **filename)
{
	size_t len;
	strlen_local(path, &len);

	*filename = path;
	for (size_t i = 0; i < len; i++) {
		if (path[i] == '/') {
			*filename = &path[i + 1];
		}
	}
}

AsyncResult fun_register_file_change_notification(String filePath,
												  FileChangeCallback callback)
{
	if (!filePath || !callback) {
		return (AsyncResult){ .status = ASYNC_ERROR,
							  .error = ERROR_RESULT_NULL_POINTER };
	}

	MemoryResult stateResult =
		fun_memory_allocate(sizeof(FileNotificationState));
	if (fun_error_is_error(stateResult.error)) {
		return (AsyncResult){ .status = ASYNC_ERROR,
							  .error = stateResult.error };
	}

	FileNotificationState *state = (FileNotificationState *)stateResult.value;
	state->file_path = filePath;
	state->callback = callback;
	state->inotify_fd_valid = false;
	state->watch_fd_valid = false;
	state->monitoring_active = true;

	int inotify_fd = (int)syscall1(SYS_inotify_init, 0);
	if (inotify_fd < 0) {
		fun_memory_free((Memory *)&state);
		return (AsyncResult){ .status = ASYNC_ERROR,
							  .error = fun_error_result(
								  -inotify_fd, "Failed to init inotify") };
	}
	state->inotify_fd = inotify_fd;
	state->inotify_fd_valid = true;

	char dir_path[512];
	get_parent_dir(filePath, dir_path);

	uint32_t mask = IN_MODIFY | IN_CREATE | IN_DELETE | IN_MOVED_FROM |
					IN_MOVED_TO;
	int watch_fd =
		(int)syscall3(SYS_inotify_add_watch, inotify_fd, (long)dir_path, mask);
	if (watch_fd < 0) {
		syscall1(SYS_close, inotify_fd);
		fun_memory_free((Memory *)&state);
		return (AsyncResult){ .status = ASYNC_ERROR,
							  .error = fun_error_result(
								  -watch_fd, "Failed to add inotify watch") };
	}
	state->watch_fd = watch_fd;
	state->watch_fd_valid = true;

	return (AsyncResult){ .state = state,
						  .status = ASYNC_PENDING,
						  .error = ERROR_RESULT_NO_ERROR };
}

AsyncResult fun_unregister_file_change_notification(void *state)
{
	if (!state) {
		return (AsyncResult){ .status = ASYNC_ERROR,
							  .error = ERROR_RESULT_NULL_POINTER };
	}

	FileNotificationState *s = (FileNotificationState *)state;

	if (s->watch_fd_valid) {
		syscall2(SYS_inotify_rm_watch, s->inotify_fd, s->watch_fd);
		s->watch_fd_valid = false;
	}

	if (s->inotify_fd_valid) {
		syscall1(SYS_close, s->inotify_fd);
		s->inotify_fd_valid = false;
	}

	fun_memory_free((Memory *)&state);

	return (AsyncResult){ .status = ASYNC_COMPLETED,
						  .error = ERROR_RESULT_NO_ERROR };
}