#include "fileRead.h"

#include <stdint.h>
#include <stddef.h>

#define NULL ((void *)0)

typedef long ssize_t;
typedef unsigned long size_t;
typedef long off_t;

#define SYS_open 2
#define SYS_close 3
#define SYS_fstat 5
#define SYS_mmap 9
#define SYS_munmap 11

#define O_RDONLY 0

#define PROT_READ 0x1
#define PROT_WRITE 0x2
#define MAP_PRIVATE 0x2
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

AsyncStatus poll_mmap(AsyncResult *result)
{
	MMapState *state = (MMapState *)result->state;
	AsyncStatus final_status = ASYNC_COMPLETED;

	if (state->file_descriptor == 0) {
		int fd = (int)syscall2(SYS_open, (long)state->parameters.file_path,
							   O_RDONLY);
		if (fd < 0) {
			result->error = fun_error_result(-fd, "Failed to open file");
			final_status = ASYNC_ERROR;
			goto cleanup;
		}
		state->file_descriptor = fd;
		return ASYNC_PENDING;
	}

	if (!state->mapped_address) {
		struct stat file_stat;
		if (syscall2(SYS_fstat, state->file_descriptor, (long)&file_stat) < 0) {
			result->error = fun_error_result(1, "Failed to get file size");
			final_status = ASYNC_ERROR;
			goto cleanup;
		}

		uint64_t granularity = PAGE_SIZE;
		state->adjusted_offset =
			(state->parameters.offset / granularity) * granularity;

		uint64_t view_size =
			state->parameters.bytes_to_read +
			(state->parameters.offset - state->adjusted_offset);

		void *mapped = sys_mmap(NULL, view_size, PROT_READ, MAP_PRIVATE,
								state->file_descriptor,
								(off_t)state->adjusted_offset);
		if (mapped == (void *)-1) {
			result->error = fun_error_result(1, "Failed to mmap file");
			final_status = ASYNC_ERROR;
			goto cleanup;
		}

		state->mapped_address = mapped;
		return ASYNC_PENDING;
	}

	uint8_t *data_ptr = (uint8_t *)state->mapped_address +
						(state->parameters.offset - state->adjusted_offset);
	voidResult copy_result = fun_memory_copy(data_ptr, state->parameters.output,
											 state->parameters.bytes_to_read);
	if (fun_error_is_error(copy_result.error)) {
		result->error = copy_result.error;
		final_status = ASYNC_ERROR;
		goto cleanup;
	}

cleanup:
	if (state) {
		if (state->mapped_address && state->mapped_address != (void *)-1) {
			uint64_t view_size =
				state->parameters.bytes_to_read +
				(state->parameters.offset - state->adjusted_offset);
			sys_munmap(state->mapped_address, view_size);
		}
		if (state->file_descriptor >= 0) {
			syscall1(SYS_close, state->file_descriptor);
		}
		fun_memory_free((Memory *)&state);
	}
	return final_status;
}