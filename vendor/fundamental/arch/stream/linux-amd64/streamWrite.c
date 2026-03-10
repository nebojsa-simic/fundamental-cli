#include "stream.h"

#include <stdint.h>
#include <stddef.h>

#define NULL ((void *)0)

typedef long ssize_t;

#define SYS_write 1
#define SYS_lseek 8

#define SEEK_SET 0

typedef struct {
	FileStream *stream;
	Memory data;
	uint64_t data_size;
	uint64_t bytes_written;
	bool write_complete;
} StreamWriteAsyncState;

static inline long syscall3(long n, long a1, long a2, long a3)
{
	long ret;
	__asm__ __volatile__("syscall"
						 : "=a"(ret)
						 : "a"(n), "D"(a1), "S"(a2), "d"(a3)
						 : "rcx", "r11", "memory");
	return ret;
}

static ssize_t sys_write(int fd, const void *buf, size_t count)
{
	return syscall3(SYS_write, fd, (long)buf, (long)count);
}

static long sys_lseek(int fd, long offset, int whence)
{
	return syscall3(SYS_lseek, fd, offset, whence);
}

static AsyncStatus poll_stream_write(AsyncResult *result)
{
	StreamWriteAsyncState *state = (StreamWriteAsyncState *)result->state;

	if (!state || !state->stream || !state->data) {
		result->error = ERROR_RESULT_NULL_POINTER;
		result->status = ASYNC_ERROR;
		return ASYNC_ERROR;
	}

	if (state->write_complete) {
		return result->status;
	}

	StreamReadState *stream_state =
		(StreamReadState *)state->stream->internal_state;
	if (!stream_state || stream_state->file_descriptor < 0) {
		result->error =
			fun_error_result(3, "Stream not properly opened for write");
		result->status = ASYNC_ERROR;
		state->write_complete = true;
		return ASYNC_ERROR;
	}

	// Set file position
	long seek_result = sys_lseek(stream_state->file_descriptor,
								 (long)state->stream->current_position,
								 SEEK_SET);
	if (seek_result < 0) {
		result->error =
			fun_error_result(1, "Failed to set file write position");
		result->status = ASYNC_ERROR;
		state->write_complete = true;
		return ASYNC_ERROR;
	}

	// Write data
	ssize_t bytes_written_sys = sys_write(
		stream_state->file_descriptor, state->data, (size_t)state->data_size);
	if (bytes_written_sys < 0) {
		result->error = fun_error_result(1, "Failed to write file");
		result->status = ASYNC_ERROR;
		state->write_complete = true;
		return ASYNC_ERROR;
	}

	// Update stream state
	state->bytes_written = (uint64_t)bytes_written_sys;
	state->stream->current_position += (uint64_t)bytes_written_sys;
	state->stream->bytes_processed += (uint64_t)bytes_written_sys;

	// Check if all data was written
	if ((uint64_t)bytes_written_sys < state->data_size) {
		result->error = fun_error_result(1, "Unable to write all data");
		result->status = ASYNC_ERROR;
		state->write_complete = true;
		return ASYNC_ERROR;
	}

	state->write_complete = true;
	result->error = ERROR_RESULT_NO_ERROR;
	result->status = ASYNC_COMPLETED;
	return ASYNC_COMPLETED;
}

AsyncResult fun_stream_write(FileStream *stream, Memory data,
							 uint64_t data_size)
{
	if (!stream || !data) {
		return (AsyncResult){ .status = ASYNC_ERROR,
							  .error = ERROR_RESULT_NULL_POINTER };
	}

	// Allocate async state
	MemoryResult state_result =
		fun_memory_allocate(sizeof(StreamWriteAsyncState));
	if (fun_error_is_error(state_result.error)) {
		return (AsyncResult){ .status = ASYNC_ERROR,
							  .error = state_result.error };
	}

	StreamWriteAsyncState *state = (StreamWriteAsyncState *)state_result.value;
	*state = (StreamWriteAsyncState){ .stream = stream,
									  .data = data,
									  .data_size = data_size,
									  .bytes_written = 0,
									  .write_complete = false };

	// Return pending result with poll function
	AsyncResult result = { .poll = poll_stream_write,
						   .state = state,
						   .status = ASYNC_PENDING,
						   .error = ERROR_RESULT_NO_ERROR };

	// Immediately poll to do the actual write
	result.status = poll_stream_write(&result);

	// If completed, free the state
	if (result.status == ASYNC_COMPLETED || result.status == ASYNC_ERROR) {
		void *state_memory = state;
		fun_memory_free(&state_memory);
	}

	return result;
}
