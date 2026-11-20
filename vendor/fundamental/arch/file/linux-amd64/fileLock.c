#include "fundamental/file/file.h"
#include "fundamental/memory/memory.h"
#include "fundamental/error/error.h"
#include "syscall_nums.h"

#include <stdint.h>
#include <stddef.h>

#define FILE_LOCK_TIMEOUT_MS 5000
#define FILE_LOCK_RETRY_INTERVAL_MS 100

struct timespec_local {
	long tv_sec;
	long tv_nsec;
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

static long get_monotonic_ms(void)
{
	struct timespec_local ts;
	syscall2(SYS_clock_gettime, CLOCK_MONOTONIC, (long)&ts);
	return ts.tv_sec * 1000L + ts.tv_nsec / 1000000L;
}

ErrorResult fun_file_lock_with_timeout(String filePath, uint32_t timeout_ms,
									   FileLockHandle *outLockHandle)
{
	if (!filePath || !outLockHandle)
		return ERROR_RESULT_NULL_POINTER;

	int fd = (int)syscall2(SYS_open, (long)filePath, O_RDWR);
	if (fd < 0)
		return fun_error_result(-fd, "Failed to open file for locking");

	long start_ms = get_monotonic_ms();

	while (1) {
		long ret = syscall2(SYS_flock, fd, LOCK_EX | LOCK_NB);
		if (ret == 0) {
			outLockHandle->state = (void *)(long)fd;
			return ERROR_RESULT_NO_ERROR;
		}
		if (ret != -EAGAIN) {
			syscall1(SYS_close, fd);
			return fun_error_result(-ret, "Failed to acquire file lock");
		}
		long elapsed_ms = get_monotonic_ms() - start_ms;
		if (elapsed_ms < 0 || (uint32_t)elapsed_ms >= timeout_ms) {
			syscall1(SYS_close, fd);
			return ERROR_RESULT_LOCK_TIMEOUT;
		}
		struct timespec_local sleep_ts = { 0, FILE_LOCK_RETRY_INTERVAL_MS *
												  1000000L };
		syscall2(SYS_nanosleep, (long)&sleep_ts, 0);
	}
}

ErrorResult fun_lock_file(String filePath, FileLockHandle *outLockHandle)
{
	return fun_file_lock_with_timeout(filePath, FILE_LOCK_TIMEOUT_MS,
									  outLockHandle);
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