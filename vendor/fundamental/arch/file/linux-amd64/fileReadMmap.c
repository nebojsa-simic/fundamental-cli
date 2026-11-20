#include "fileRead.h"
#include "fileAdaptive.h"
#include "syscall_nums.h"
#include "page_size.h"

#include <stdint.h>
#include <stddef.h>

typedef unsigned long size_t;
typedef long off_t;

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
	FileAdaptiveState *adaptive = state->parameters.adaptive;
	uint64_t bytes = state->parameters.bytes_to_read;
	AsyncStatus final_status = ASYNC_COMPLETED;

	if (!state->fd_valid) {
		int fd = (int)syscall2(SYS_open, (long)state->parameters.file_path,
							   O_RDONLY);
		if (fd < 0) {
			result->error = fun_error_result(-fd, "Failed to open file");
			final_status = ASYNC_ERROR;
			goto cleanup;
		}
		state->file_descriptor = fd;
		state->fd_valid = true;
		return ASYNC_PENDING;
	}

	if (!state->mmap_valid) {
		struct stat file_stat;
		if (syscall2(SYS_fstat, state->file_descriptor, (long)&file_stat) < 0) {
			result->error = fun_error_result(1, "Failed to get file size");
			final_status = ASYNC_ERROR;
			goto cleanup;
		}

		/* off_t is a signed 64-bit type; reject offsets that overflow it. */
		if (state->parameters.offset > (uint64_t)INT64_MAX) {
			result->error = ERROR_RESULT_INTEGER_OVERFLOW;
			final_status = ASYNC_ERROR;
			goto cleanup;
		}

		uint64_t granularity = get_page_size();
		state->adjusted_offset =
			(state->parameters.offset / granularity) * granularity;
		uint64_t intra_page_offset =
			state->parameters.offset - state->adjusted_offset;

		if (state->parameters.bytes_to_read > UINT64_MAX - intra_page_offset) {
			result->error = ERROR_RESULT_INTEGER_OVERFLOW;
			final_status = ASYNC_ERROR;
			goto cleanup;
		}
		uint64_t view_size =
			state->parameters.bytes_to_read + intra_page_offset;

		void *mapped = sys_mmap(NULL, view_size, PROT_READ, MAP_PRIVATE,
								state->file_descriptor,
								(off_t)state->adjusted_offset);
		if ((long)mapped < 0 && (long)mapped >= -4095) {
			result->error = fun_error_result(1, "Failed to mmap file");
			final_status = ASYNC_ERROR;
			goto cleanup;
		}

		state->mapped_address = mapped;
		state->mmap_valid = true;
		return ASYNC_PENDING;
	}

	uint8_t *data_ptr = (uint8_t *)state->mapped_address +
						(state->parameters.offset - state->adjusted_offset);
	voidResult copy_result = fun_memory_copy(data_ptr, state->parameters.output,
											 state->parameters.bytes_to_read);
	if (fun_error_is_error(copy_result.error)) {
		result->error = copy_result.error;
		final_status = ASYNC_ERROR;
	}

cleanup:
	if (state) {
		if (state->mmap_valid) {
			uint64_t view_size =
				state->parameters.bytes_to_read +
				(state->parameters.offset - state->adjusted_offset);
			sys_munmap(state->mapped_address, view_size);
		}
		if (state->fd_valid)
			syscall1(SYS_close, state->file_descriptor);
		fun_memory_free((Memory *)&state);
	}
	if (final_status == ASYNC_COMPLETED)
		file_adaptive_update(adaptive, bytes);
	return final_status;
}
