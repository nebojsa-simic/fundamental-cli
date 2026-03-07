#include "stream.h"
#include <windows.h>

typedef struct {
	FileStream *stream;
	Memory data;
	uint64_t data_size;
	uint64_t bytes_written;
	bool write_complete;
} StreamWriteAsyncState;

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
	if (!stream_state || stream_state->file_handle == INVALID_HANDLE_VALUE) {
		result->error =
			fun_error_result(3, "Stream not properly opened for write");
		result->status = ASYNC_ERROR;
		state->write_complete = true;
		return ASYNC_ERROR;
	}

	// Set file pointer to current position
	LARGE_INTEGER position;
	position.QuadPart = state->stream->current_position;
	if (!SetFilePointerEx(stream_state->file_handle, position, NULL,
						  FILE_BEGIN)) {
		result->error = fun_error_result(GetLastError(),
										 "Failed to set file write position");
		result->status = ASYNC_ERROR;
		state->write_complete = true;
		return ASYNC_ERROR;
	}

	// Write data to file
	DWORD bytes_written_win;
	if (!WriteFile(stream_state->file_handle, state->data,
				   (DWORD)state->data_size, &bytes_written_win, NULL)) {
		result->error =
			fun_error_result(GetLastError(), "Failed to write file");
		result->status = ASYNC_ERROR;
		state->write_complete = true;
		return ASYNC_ERROR;
	}

	// Update stream state
	state->bytes_written = bytes_written_win;
	state->stream->current_position += bytes_written_win;
	state->stream->bytes_processed += bytes_written_win;

	// If we couldn't write all data, that might indicate an issue
	if (bytes_written_win < state->data_size) {
		result->error =
			fun_error_result(GetLastError(), "Unable to write all data");
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
		fun_memory_free((Memory *)&state);
	}

	return result;
}

if (state->write_complete) {
	return result->status;
}

StreamReadState *stream_state =
	(StreamReadState *)state->stream->internal_state;
if (!stream_state || stream_state->file_handle == INVALID_HANDLE_VALUE) {
	result->error = fun_error_result(3, "Stream not properly opened for write");
	result->status = ASYNC_ERROR;
	state->write_complete = true;
	return ASYNC_ERROR;
}

// Set file pointer to current position
LARGE_INTEGER position;
position.QuadPart = state->stream->current_position;
if (!SetFilePointerEx(stream_state->file_handle, position, NULL, FILE_BEGIN)) {
	result->error =
		fun_error_result(GetLastError(), "Failed to set file write position");
	result->status = ASYNC_ERROR;
	state->write_complete = true;
	return ASYNC_ERROR;
}

// Write data to file
DWORD bytes_written_win;
if (!WriteFile(stream_state->file_handle, state->data, (DWORD)state->data_size,
			   &bytes_written_win, NULL)) {
	result->error = fun_error_result(GetLastError(), "Failed to write file");
	result->status = ASYNC_ERROR;
	state->write_complete = true;
	return ASYNC_ERROR;
}

// Update stream state
state->bytes_written = bytes_written_win;
state->stream->current_position += bytes_written_win;
state->stream->bytes_processed += bytes_written_win;

// If we couldn't write all data, that might indicate an issue
if (bytes_written_win < state->data_size) {
	result->error =
		fun_error_result(GetLastError(), "Unable to write all data");
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
		fun_memory_free((Memory *)&state);
	}

	return result;
}

if (state->write_complete) {
	return result->status;
}

StreamReadState *stream_state =
	(StreamReadState *)state->stream->internal_state;
if (!stream_state || stream_state->file_handle == INVALID_HANDLE_VALUE) {
	result->error = fun_error_result(3, "Stream not properly opened for write");
	result->status = ASYNC_ERROR;
	state->write_complete = true;
	return ASYNC_ERROR;
}

// Set file pointer to current position
LARGE_INTEGER position;
position.QuadPart = state->stream->current_position;
if (!SetFilePointerEx(stream_state->file_handle, position, NULL, FILE_BEGIN)) {
	result->error =
		fun_error_result(GetLastError(), "Failed to set file write position");
	result->status = ASYNC_ERROR;
	state->write_complete = true;
	return ASYNC_ERROR;
}

// Write data to file
DWORD bytes_written_win;
if (!WriteFile(stream_state->file_handle, state->data, (DWORD)state->data_size,
			   &bytes_written_win, NULL)) {
	result->error = fun_error_result(GetLastError(), "Failed to write file");
	result->status = ASYNC_ERROR;
	state->write_complete = true;
	return ASYNC_ERROR;
}

// Update stream state
state->bytes_written = bytes_written_win;
state->stream->current_position += bytes_written_win;
state->stream->bytes_processed += bytes_written_win;

// If we couldn't write all data, that might indicate an issue
if (bytes_written_win < state->data_size) {
	result->error =
		fun_error_result(GetLastError(), "Unable to write all data");
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
		fun_memory_free((Memory *)&state);
	}

	return result;
}