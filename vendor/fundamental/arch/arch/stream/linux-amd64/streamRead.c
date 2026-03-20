#include "stream.h"

#include <stdint.h>
#include <stddef.h>

#include <stddef.h>

typedef long ssize_t;
typedef unsigned long size_t;
typedef long off_t;

#define SYS_read 0
#define SYS_close 3
#define SYS_lseek 8

#define SEEK_SET 0

typedef struct {
	FileStream *stream;
	uint64_t *bytes_read;
	bool read_complete;
} StreamReadAsyncState;

static inline long syscall1(long n, long a1)
{
	long ret;
	__asm__ __volatile__("syscall"
						 : "=a"(ret)
						 : "a"(n), "D"(a1)
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

static ssize_t sys_read(int fd, void *buf, size_t count)
{
	return (ssize_t)syscall3(SYS_read, fd, (long)buf, (long)count);
}

static off_t sys_lseek(int fd, off_t offset, int whence)
{
	return (off_t)syscall3(SYS_lseek, fd, (long)offset, whence);
}

static int sys_close(int fd)
{
	return (int)syscall1(SYS_close, fd);
}

AsyncStatus poll_stream_read(AsyncResult *result)
{
	StreamReadAsyncState *state = (StreamReadAsyncState *)result->state;

	if (state->read_complete) {
		return result->status;
	}

	if (!state->stream || !state->bytes_read) {
		result->error = ERROR_RESULT_NULL_POINTER;
		result->status = ASYNC_ERROR;
		state->read_complete = true;
		return ASYNC_ERROR;
	}

	StreamReadState *stream_state =
		(StreamReadState *)state->stream->internal_state;
	if (!stream_state || stream_state->file_descriptor < 0) {
		result->error = fun_error_result(3, "Stream not properly opened");
		result->status = ASYNC_ERROR;
		state->read_complete = true;
		return ASYNC_ERROR;
	}

	if (state->stream->end_of_stream) {
		*state->bytes_read = 0;
		result->error = ERROR_RESULT_NO_ERROR;
		result->status = ASYNC_COMPLETED;
		state->read_complete = true;
		return ASYNC_COMPLETED;
	}

	uint64_t remaining_in_file =
		stream_state->file_size - state->stream->current_position;
	uint64_t to_read = (remaining_in_file < state->stream->buffer_size) ?
						   remaining_in_file :
						   state->stream->buffer_size;

	if (to_read == 0) {
		state->stream->end_of_stream = true;
		state->stream->has_data_available = false;
		*state->bytes_read = 0;
		result->error = ERROR_RESULT_NO_ERROR;
		result->status = ASYNC_COMPLETED;
		state->read_complete = true;
		return ASYNC_COMPLETED;
	}

	off_t seek_result = sys_lseek(stream_state->file_descriptor,
								  (off_t)state->stream->current_position,
								  SEEK_SET);
	if (seek_result < 0) {
		result->error = fun_error_result(1, "Failed to set file position");
		result->status = ASYNC_ERROR;
		state->read_complete = true;
		return ASYNC_ERROR;
	}

	ssize_t bytes_read_sys = sys_read(stream_state->file_descriptor,
									  state->stream->buffer, (size_t)to_read);
	if (bytes_read_sys < 0) {
		result->error = fun_error_result(1, "Failed to read file");
		result->status = ASYNC_ERROR;
		state->read_complete = true;
		return ASYNC_ERROR;
	}

	*state->bytes_read = (uint64_t)bytes_read_sys;
	state->stream->current_position += (uint64_t)bytes_read_sys;
	state->stream->bytes_processed += (uint64_t)bytes_read_sys;

	if ((uint64_t)bytes_read_sys < to_read ||
		state->stream->current_position >= stream_state->file_size) {
		state->stream->end_of_stream = true;
		state->stream->has_data_available = false;
	}

	result->error = ERROR_RESULT_NO_ERROR;
	result->status = ASYNC_COMPLETED;
	state->read_complete = true;
	return ASYNC_COMPLETED;
}

AsyncResult fun_stream_read(FileStream *stream, uint64_t *bytes_read)
{
	if (!stream || !bytes_read) {
		return (AsyncResult){ .status = ASYNC_ERROR,
							  .error = ERROR_RESULT_NULL_POINTER };
	}

	MemoryResult state_result =
		fun_memory_allocate(sizeof(StreamReadAsyncState));
	if (fun_error_is_error(state_result.error)) {
		return (AsyncResult){ .status = ASYNC_ERROR,
							  .error = state_result.error };
	}

	StreamReadAsyncState *state = (StreamReadAsyncState *)state_result.value;
	*state = (StreamReadAsyncState){ .stream = stream,
									 .bytes_read = bytes_read,
									 .read_complete = false };

	AsyncResult result = { .poll = poll_stream_read,
						   .state = state,
						   .status = ASYNC_PENDING,
						   .error = ERROR_RESULT_NO_ERROR };

	result.status = poll_stream_read(&result);

	if (result.status == ASYNC_COMPLETED || result.status == ASYNC_ERROR) {
		fun_memory_free((Memory *)&state);
	}

	return result;
}