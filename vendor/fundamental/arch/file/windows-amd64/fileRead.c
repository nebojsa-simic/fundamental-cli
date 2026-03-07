#include "fileRead.h"

#include <windows.h>

typedef struct {
	Read parameters;
	HANDLE file_handle;
	bool file_opened;
} StandardReadState;

static AsyncStatus poll_standard_read(AsyncResult *result)
{
	StandardReadState *state = (StandardReadState *)result->state;

	if (!state->file_opened) {
		wchar_t wide_path_buffer[MAX_PATH];
		int chars_converted =
			MultiByteToWideChar(CP_UTF8, 0, state->parameters.file_path, -1,
								wide_path_buffer, MAX_PATH);
		if (chars_converted == 0) {
			result->error =
				(ErrorResult){ .code = GetLastError(),
							   .message = "MultiByteToWideChar error" };
			goto cleanup;
		}

		state->file_handle = CreateFileW(wide_path_buffer, GENERIC_READ,
										 FILE_SHARE_READ, NULL, OPEN_EXISTING,
										 FILE_ATTRIBUTE_NORMAL, NULL);
		if (state->file_handle == INVALID_HANDLE_VALUE) {
			result->error = (ErrorResult){ .code = GetLastError(),
										   .message = "CreateFileW error" };
			goto cleanup;
		}

		state->file_opened = true;
		return ASYNC_PENDING;
	}

	LARGE_INTEGER distance_to_move;
	LARGE_INTEGER new_position;
	distance_to_move.QuadPart = state->parameters.offset;

	if (!SetFilePointerEx(state->file_handle, distance_to_move, &new_position,
						  FILE_BEGIN)) {
		result->error = (ErrorResult){ .code = GetLastError(),
									   .message = "SetFilePointerEx error" };
		goto cleanup;
	}

	DWORD bytes_read = 0;
	if (!ReadFile(state->file_handle, state->parameters.output,
				  (DWORD)state->parameters.bytes_to_read, &bytes_read, NULL)) {
		result->error = (ErrorResult){ .code = GetLastError(),
									   .message = "ReadFile error" };
		goto cleanup;
	}

	fun_memory_free((Memory *)&state);
	CloseHandle(state->file_handle);

	result->error = ERROR_RESULT_NO_ERROR;
	return ASYNC_COMPLETED;

cleanup:
	if (state) {
		if (state->file_handle != INVALID_HANDLE_VALUE) {
			CloseHandle(state->file_handle);
		}
		fun_memory_free((Memory *)&state);
	}
	return ASYNC_ERROR;
}

static AsyncResult create_standard_read(Read parameters)
{
	StandardReadState *state =
		(StandardReadState *)fun_memory_allocate(sizeof(StandardReadState))
			.value;
	*state = (StandardReadState){ .parameters = parameters,
								  .file_handle = INVALID_HANDLE_VALUE,
								  .file_opened = false };

	return (AsyncResult){
		.state = state,
		.poll = poll_standard_read,
		.status = ASYNC_PENDING,
	};
}

AsyncResult fun_read_file_in_memory(Read parameters)
{
	switch (parameters.mode) {
	case FILE_MODE_STANDARD:
		return create_standard_read(parameters);
	case FILE_MODE_MMAP:
	case FILE_MODE_AUTO:
	default: {
		MMapState *state =
			(MMapState *)fun_memory_allocate(sizeof(MMapState)).value;
		*state = (MMapState){ .parameters = parameters };

		return (AsyncResult){
			.state = state,
			.poll = poll_mmap,
			.status = ASYNC_PENDING,
		};
	}
	}
}