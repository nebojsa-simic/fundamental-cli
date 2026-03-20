#include "fundamental/stream/stream.h"
#include "fundamental/memory/memory.h"

/* Arch-layer declaration (implemented per platform in arch/stream/) */
extern void arch_stream_close_handle(void *internal_state);

AsyncResult fun_stream_close(FileStream *stream)
{
	if (!stream) {
		return (AsyncResult){ .status = ASYNC_ERROR,
							  .error = ERROR_RESULT_NULL_POINTER };
	}

	void *state = stream->internal_state;
	if (state) {
		arch_stream_close_handle(state);
		fun_memory_free((Memory *)&state);
	}

	fun_memory_free((Memory *)&stream);
	return (AsyncResult){ .status = ASYNC_COMPLETED,
						  .error = ERROR_RESULT_NO_ERROR };
}
