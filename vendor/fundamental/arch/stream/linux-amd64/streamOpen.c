#include "stream.h"

#include <stdint.h>
#include <stddef.h>

#define NULL ((void *)0)

typedef long ssize_t;
typedef unsigned long size_t;

#define SYS_open 2
#define SYS_close 3
#define SYS_fstat 5
#define SYS_lseek 8

#define O_RDONLY 0
#define O_WRONLY 1
#define O_RDWR 2
#define O_CREAT 0100
#define O_TRUNC 01000
#define O_APPEND 02000

#define SEEK_SET 0
#define SEEK_END 2

struct stat {
	unsigned long st_dev;
	unsigned long st_ino;
	unsigned long st_nlink;
	unsigned int st_mode;
	unsigned int st_uid;
	unsigned int st_gid;
	unsigned long st_rdev;
	unsigned long st_size;
	unsigned long st_blksize;
	unsigned long st_blocks;
	unsigned long st_atime;
	unsigned long st_atime_nsec;
	unsigned long st_mtime;
	unsigned long st_mtime_nsec;
	unsigned long st_ctime;
	unsigned long st_ctime_nsec;
	unsigned long __unused[3];
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

AsyncResult fun_stream_open(String file_path, StreamMode mode, Memory buffer,
							uint64_t buffer_size, FileMode file_mode)
{
	if (!file_path || !buffer || buffer_size == 0) {
		return (AsyncResult){ .status = ASYNC_ERROR,
							  .error = ERROR_RESULT_NULL_POINTER };
	}

	MemoryResult stream_result = fun_memory_allocate(sizeof(FileStream));
	if (fun_error_is_error(stream_result.error)) {
		return (AsyncResult){ .status = ASYNC_ERROR,
							  .error = stream_result.error };
	}

	FileStream *stream = (FileStream *)stream_result.value;
	*stream = (FileStream){ .file_path = file_path,
							.mode = mode,
							.buffer = buffer,
							.buffer_size = buffer_size,
							.current_position = 0,
							.bytes_processed = 0,
							.end_of_stream = false,
							.has_data_available = (mode == STREAM_MODE_READ),
							.internal_state = NULL };

	MemoryResult state_result = fun_memory_allocate(sizeof(StreamReadState));
	if (fun_error_is_error(state_result.error)) {
		fun_memory_free((Memory *)&stream);
		return (AsyncResult){ .status = ASYNC_ERROR,
							  .error = state_result.error };
	}

	StreamReadState *state = (StreamReadState *)state_result.value;
	*state = (StreamReadState){ .stream = stream,
								.file_descriptor = -1,
								.file_size = 0,
								.file_opened = false };

	stream->internal_state = state;

	return (AsyncResult){ .poll = poll_stream_open,
						  .state = stream,
						  .status = ASYNC_PENDING,
						  .error = ERROR_RESULT_NO_ERROR };
}

AsyncStatus poll_stream_open(AsyncResult *result)
{
	FileStream *stream = (FileStream *)result->state;
	StreamReadState *state = (StreamReadState *)stream->internal_state;

	if (!state->file_opened) {
		int flags;
		if (stream->mode == STREAM_MODE_READ) {
			flags = O_RDONLY;
		} else if (stream->mode == STREAM_MODE_WRITE) {
			flags = O_WRONLY | O_CREAT | O_TRUNC;
		} else {
			flags = O_WRONLY | O_CREAT | O_APPEND;
		}

		int fd;
		if (flags & O_CREAT) {
			fd = (int)syscall3(SYS_open, (long)stream->file_path, flags, 0644);
		} else {
			fd = (int)syscall2(SYS_open, (long)stream->file_path, flags);
		}
		if (fd < 0) {
			result->error = fun_error_result(-fd, "Failed to open file");
			return ASYNC_ERROR;
		}

		state->file_descriptor = fd;

		if (stream->mode == STREAM_MODE_READ) {
			struct stat file_stat;
			if (syscall2(SYS_fstat, fd, (long)&file_stat) < 0) {
				syscall1(SYS_close, fd);
				result->error = fun_error_result(1, "Failed to get file size");
				return ASYNC_ERROR;
			}
			state->file_size = file_stat.st_size;
		}

		state->file_opened = true;
		return ASYNC_PENDING;
	}

	result->error = ERROR_RESULT_NO_ERROR;
	return ASYNC_COMPLETED;
}

void arch_stream_close_handle(void *internal_state)
{
	StreamReadState *state = (StreamReadState *)internal_state;
	if (state && state->file_descriptor != -1) {
		syscall1(SYS_close, state->file_descriptor);
	}
}