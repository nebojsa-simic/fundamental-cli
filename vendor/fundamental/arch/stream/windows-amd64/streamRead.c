#include "stream.h"

typedef struct {
	FileStream *stream;
	uint64_t *bytes_read;
	bool read_complete;
} StreamReadAsyncState;

AsyncStatus poll_stream_read(AsyncResult *result)
{
	StreamReadAsyncState *state = (StreamReadAsyncState *)result->state;

	if (state->read_complete) {
		return result->status;
	}

	// Perform the actual read operation
	if (!state->stream || !state->bytes_read) {
		result->error = ERROR_RESULT_NULL_POINTER;
		result->status = ASYNC_ERROR;
		state->read_complete = true;
		return ASYNC_ERROR;
	}

	StreamReadState *stream_state =
		(StreamReadState *)state->stream->internal_state;
	if (!stream_state || stream_state->file_handle == INVALID_HANDLE_VALUE) {
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

	// Calculate how much to read
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

	// Set file pointer to current position
	LARGE_INTEGER position;
	position.QuadPart = state->stream->current_position;
	if (!SetFilePointerEx(stream_state->file_handle, position, NULL,
						  FILE_BEGIN)) {
		result->error =
			fun_error_result(GetLastError(), "Failed to set file position");
		result->status = ASYNC_ERROR;
		state->read_complete = true;
		return ASYNC_ERROR;
	}

	// Read data into buffer
	DWORD bytes_read_win;
	if (!ReadFile(stream_state->file_handle, state->stream->buffer,
				  (DWORD)to_read, &bytes_read_win, NULL)) {
		result->error = fun_error_result(GetLastError(), "Failed to read file");
		result->status = ASYNC_ERROR;
		state->read_complete = true;
		return ASYNC_ERROR;
	}

	// Update stream state
	*state->bytes_read = bytes_read_win;
	state->stream->current_position += bytes_read_win;
	state->stream->bytes_processed += bytes_read_win;

	// Check for end of stream
	if (bytes_read_win < to_read ||
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

	// Allocate async state
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

	// Return pending result with poll function
	AsyncResult result = { .poll = poll_stream_read,
						   .state = state,
						   .status = ASYNC_PENDING,
						   .error = ERROR_RESULT_NO_ERROR };

	// Immediately poll to do the actual read
	result.status = poll_stream_read(&result);

	// If completed, free the state
	if (result.status == ASYNC_COMPLETED || result.status == ASYNC_ERROR) {
		fun_memory_free((Memory *)&state);
	}

	return result;
}
