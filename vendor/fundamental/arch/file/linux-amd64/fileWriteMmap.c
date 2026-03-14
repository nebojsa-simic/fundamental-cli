#include "fileWrite.h"
#include "fileAdaptive.h"

#include <stdint.h>
#include <stddef.h>

#define NULL ((void *)0)

typedef long ssize_t;
typedef unsigned long size_t;
typedef long off_t;

#define SYS_write 1
#define SYS_open 2
#define SYS_close 3
#define SYS_fstat 5
#define SYS_mmap 9
#define SYS_munmap 11
#define SYS_ftruncate 77

#define O_RDWR 2
#define O_CREAT 0100

#define PROT_READ 0x1
#define PROT_WRITE 0x2
#define MAP_SHARED 0x1

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

#define PAGE_SIZE 4096

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

static inline void *sys_mmap(void *addr, size_t length, int prot, int flags,
							 int fd, off_t offset)
{
	return (void *)syscall6(SYS_mmap, (long)addr, (long)length, prot, flags, fd,
							offset);
}

static inline int sys_munmap(void *addr, size_t length)
{
	return (int)syscall2(SYS_munmap, (long)addr, (long)length);
}

static inline int sys_ftruncate(int fd, off_t length)
{
	return (int)syscall2(SYS_ftruncate, fd, (long)length);
}

AsyncStatus poll_mmap_write(AsyncResult *result)
{
	MMapWriteState *state = (MMapWriteState *)result->state;
	FileAdaptiveState *adaptive = state->parameters.adaptive;
	uint64_t bytes = state->parameters.bytes_to_write;
	AsyncStatus final_status = ASYNC_COMPLETED;

	if (state->file_descriptor < 0) {
		int fd = (int)syscall3(SYS_open, (long)state->parameters.file_path,
							   O_RDWR | O_CREAT, 0644);
		if (fd < 0) {
			result->error = fun_error_result(-fd, "Failed to open/create file");
			return ASYNC_ERROR;
		}
		state->file_descriptor = fd;
		return ASYNC_PENDING;
	}

	if (!state->file_extended) {
		struct stat file_stat;
		if (syscall2(SYS_fstat, state->file_descriptor, (long)&file_stat) < 0) {
			result->error = fun_error_result(1, "Failed to get file size");
			final_status = ASYNC_ERROR;
			goto cleanup;
		}

		state->original_file_size = file_stat.st_size;
		uint64_t required_size =
			state->parameters.offset + state->parameters.bytes_to_write;

		if (required_size > state->original_file_size) {
			if (sys_ftruncate(state->file_descriptor, (off_t)required_size) <
				0) {
				result->error = fun_error_result(1, "Failed to extend file");
				final_status = ASYNC_ERROR;
				goto cleanup;
			}
		}
		state->file_extended = true;
		return ASYNC_PENDING;
	}

	if (!state->mapped_address) {
		uint64_t granularity = PAGE_SIZE;
		state->adjusted_offset =
			(state->parameters.offset / granularity) * granularity;
		uint64_t view_size =
			state->parameters.bytes_to_write +
			(state->parameters.offset - state->adjusted_offset);

		void *mapped = sys_mmap(NULL, view_size, PROT_READ | PROT_WRITE,
								MAP_SHARED, state->file_descriptor,
								(off_t)state->adjusted_offset);
		if (mapped == (void *)-1) {
			result->error =
				fun_error_result(1, "Failed to mmap file for write");
			final_status = ASYNC_ERROR;
			goto cleanup;
		}
		state->mapped_address = mapped;
		return ASYNC_PENDING;
	}

	uint64_t actual_offset = state->parameters.offset - state->adjusted_offset;
	void *write_location = (char *)state->mapped_address + actual_offset;
	fun_memory_copy(state->parameters.input, write_location,
					state->parameters.bytes_to_write);

cleanup:
	if (state->mapped_address && state->mapped_address != (void *)-1) {
		uint64_t view_size =
			state->parameters.bytes_to_write +
			(state->parameters.offset - state->adjusted_offset);
		sys_munmap(state->mapped_address, view_size);
	}
	if (state->file_descriptor >= 0)
		syscall1(SYS_close, state->file_descriptor);
	fun_memory_free((Memory *)&state);
	if (final_status == ASYNC_COMPLETED)
		file_adaptive_update(adaptive, bytes);
	return final_status;
}
