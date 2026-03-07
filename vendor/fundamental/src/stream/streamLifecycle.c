#include "stream/stream.h"
#include "memory/memory.h"
#include "../arch/stream/windows-amd64/stream.h"

AsyncResult fun_stream_close(FileStream *stream)
{
	if (!stream) {
		return (AsyncResult){ .status = ASYNC_ERROR,
							  .error = ERROR_RESULT_NULL_POINTER };
	}

	StreamReadState *state = (StreamReadState *)stream->internal_state;
	if (state) {
		if (state->file_handle != INVALID_HANDLE_VALUE) {
			CloseHandle(state->file_handle);
		}
		fun_memory_free((Memory *)&state);
	}

	fun_memory_free((Memory *)&stream);
	return (AsyncResult){ .status = ASYNC_COMPLETED,
						  .error = ERROR_RESULT_NO_ERROR };
}
