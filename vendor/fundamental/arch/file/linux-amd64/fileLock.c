#include "fundamental/file/file.h"
#include "fundamental/memory/memory.h"
#include "fundamental/error/error.h"

#include <stdint.h>
#include <stddef.h>

#define NULL ((void *)0)

#define SYS_open 2
#define SYS_close 3
#define SYS_flock 73

#define LOCK_SH 1
#define LOCK_EX 2
#define LOCK_UN 8
#define LOCK_NB 4

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

#define O_RDONLY 0
#define O_WRONLY 1
#define O_RDWR 2

ErrorResult fun_lock_file(String filePath, FileLockHandle *outLockHandle)
{
	if (!filePath || !outLockHandle) {
		return ERROR_RESULT_NULL_POINTER;
	}

	int fd = (int)syscall2(SYS_open, (long)filePath, O_RDWR);
	if (fd < 0) {
		return fun_error_result(-fd, "Failed to open file for locking");
	}

	long result = syscall2(SYS_flock, fd, LOCK_EX);
	if (result < 0) {
		syscall1(SYS_close, fd);
		return fun_error_result(-result, "Failed to acquire file lock");
	}

	outLockHandle->state = (void *)(long)fd;
	return ERROR_RESULT_NO_ERROR;
}

ErrorResult fun_unlock_file(FileLockHandle lockHandle)
{
	if (!lockHandle.state) {
		return ERROR_RESULT_NULL_POINTER;
	}

	int fd = (int)(long)lockHandle.state;

	long result = syscall2(SYS_flock, fd, LOCK_UN);
	syscall1(SYS_close, fd);

	if (result < 0) {
		return fun_error_result(-result, "Failed to release file lock");
	}

	return ERROR_RESULT_NO_ERROR;
}