#include <windows.h>
#include "file/file.h"
#include "memory/memory.h"
#include "error/error.h"

typedef struct {
	Append parameters;
	HANDLE file_handle;
	bool file_opened;
} StandardAppendState;

static AsyncStatus poll_standard_append(AsyncResult *result)
{
	StandardAppendState *state = (StandardAppendState *)result->state;

	if (!state->file_opened) {
		wchar_t wide_path_buffer[MAX_PATH];
		int chars_converted =
			MultiByteToWideChar(CP_UTF8, 0, state->parameters.file_path, -1,
								wide_path_buffer, MAX_PATH);
		if (chars_converted == 0) {
			result->error =
				(ErrorResult){ .code = GetLastError(),
							   .message = "MultiByteToWideChar error" };
			return ASYNC_ERROR;
		}

		// Open file in append mode
		state->file_handle =
			CreateFileW(wide_path_buffer, GENERIC_WRITE, FILE_SHARE_READ, NULL,
						OPEN_ALWAYS, // Open existing or create new
						FILE_ATTRIBUTE_NORMAL, NULL);
		if (state->file_handle == INVALID_HANDLE_VALUE) {
			result->error = (ErrorResult){ .code = GetLastError(),
										   .message = "CreateFileW error" };
			return ASYNC_ERROR;
		}

		// Seek to end of file to append
		LARGE_INTEGER distance_to_move;
		LARGE_INTEGER new_position;
		distance_to_move.QuadPart = 0;

		if (!SetFilePointerEx(state->file_handle, distance_to_move,
							  &new_position, FILE_END)) {
			result->error =
				(ErrorResult){ .code = GetLastError(),
							   .message = "SetFilePointerEx error" };
			CloseHandle(state->file_handle);
			return ASYNC_ERROR;
		}

		state->file_opened = true;
		return ASYNC_PENDING;
	}

	// Write data at the end of file
	DWORD bytes_written = 0;
	if (!WriteFile(state->file_handle, state->parameters.input,
				   (DWORD)state->parameters.bytes_to_append, &bytes_written,
				   NULL)) {
		result->error = (ErrorResult){ .code = GetLastError(),
									   .message = "WriteFile append error" };
		CloseHandle(state->file_handle);
		return ASYNC_ERROR;
	}

	// Verify all bytes were written
	if (bytes_written != state->parameters.bytes_to_append) {
		result->error =
			(ErrorResult){ .code = GetLastError(),
						   .message = "Append: not all bytes written" };
		CloseHandle(state->file_handle);
		return ASYNC_ERROR;
	}

	// Cleanup
	CloseHandle(state->file_handle);
	fun_memory_free((Memory *)&state);

	result->error = ERROR_RESULT_NO_ERROR;
	return ASYNC_COMPLETED;
}

static AsyncResult create_standard_append(Append parameters)
{
	StandardAppendState *state =
		(StandardAppendState *)fun_memory_allocate(sizeof(StandardAppendState))
			.value;
	*state = (StandardAppendState){ .parameters = parameters,
									.file_handle = INVALID_HANDLE_VALUE,
									.file_opened = false };

	return (AsyncResult){
		.state = state,
		.poll = poll_standard_append,
		.status = ASYNC_PENDING,
	};
}

AsyncResult fun_append_memory_to_file(Append parameters)
{
	// Choose implementation based on specified mode
	switch (parameters.mode) {
	case FILE_MODE_STANDARD:
		return create_standard_append(parameters);
	case FILE_MODE_MMAP:
	case FILE_MODE_AUTO:
	default: {
		// For now, use standard implementation as default for auto
		return create_standard_append(parameters);
	}
	}
}