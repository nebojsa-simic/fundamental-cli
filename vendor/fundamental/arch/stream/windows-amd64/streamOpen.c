// arch/stream/windows-amd64/streamRead.c
#include "stream.h"

AsyncResult fun_stream_open(String file_path, StreamMode mode, Memory buffer,
							uint64_t buffer_size, FileMode file_mode)
{
	// Validate parameters
	if (!file_path || !buffer || buffer_size == 0) {
		return (AsyncResult){ .status = ASYNC_ERROR,
							  .error = ERROR_RESULT_NULL_POINTER };
	}

	// Allocate stream structure
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

	// Allocate platform-specific state
	MemoryResult state_result = fun_memory_allocate(sizeof(StreamReadState));
	if (fun_error_is_error(state_result.error)) {
		fun_memory_free((Memory *)&stream);
		return (AsyncResult){ .status = ASYNC_ERROR,
							  .error = state_result.error };
	}

	StreamReadState *state = (StreamReadState *)state_result.value;
	*state = (StreamReadState){ .stream = stream,
								.file_handle = INVALID_HANDLE_VALUE,
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

	// Step 1: Open file if not already opened
	if (!state->file_opened) {
		// Convert UTF-8 path to wide string
		wchar_t wide_path[MAX_PATH];
		int chars_converted = MultiByteToWideChar(CP_UTF8, 0, stream->file_path,
												  -1, wide_path, MAX_PATH);
		if (chars_converted == 0) {
			result->error =
				fun_error_result(GetLastError(), "Path conversion failed");
			return ASYNC_ERROR;
		}

		// Open file with appropriate access
		DWORD access = (stream->mode == STREAM_MODE_READ) ? GENERIC_READ :
															GENERIC_WRITE;
		DWORD creation = (stream->mode == STREAM_MODE_WRITE) ? CREATE_ALWAYS :
															   OPEN_EXISTING;

		state->file_handle = CreateFileW(wide_path, access, FILE_SHARE_READ,
										 NULL, creation, FILE_ATTRIBUTE_NORMAL,
										 NULL);

		if (state->file_handle == INVALID_HANDLE_VALUE) {
			result->error =
				fun_error_result(GetLastError(), "Failed to open file");
			return ASYNC_ERROR;
		}

		// Get file size for read operations
		if (stream->mode == STREAM_MODE_READ) {
			LARGE_INTEGER file_size;
			if (!GetFileSizeEx(state->file_handle, &file_size)) {
				CloseHandle(state->file_handle);
				result->error =
					fun_error_result(GetLastError(), "Failed to get file size");
				return ASYNC_ERROR;
			}
			state->file_size = file_size.QuadPart;
		}

		state->file_opened = true;
		return ASYNC_PENDING; // Continue to next step
	}

	// File is opened and ready for operations
	result->error = ERROR_RESULT_NO_ERROR;
	return ASYNC_COMPLETED;
}
