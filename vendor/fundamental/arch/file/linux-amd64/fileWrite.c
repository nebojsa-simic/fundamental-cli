#include "fileWrite.h"

#include <stdint.h>
#include <stddef.h>

#define NULL ((void *)0)

AsyncResult fun_write_memory_to_file(Write parameters)
{
	if (!parameters.file_path || !parameters.input) {
		return (AsyncResult){ .status = ASYNC_ERROR,
							  .error = ERROR_RESULT_NULL_POINTER };
	}

	if (parameters.bytes_to_write == 0) {
		return (AsyncResult){ .status = ASYNC_COMPLETED,
							  .error = ERROR_RESULT_NO_ERROR };
	}

	MemoryResult allocation = fun_memory_allocate(sizeof(MMapWriteState));
	if (fun_error_is_error(allocation.error)) {
		return (AsyncResult){ .status = ASYNC_ERROR,
							  .error = allocation.error };
	}

	MMapWriteState *state = (MMapWriteState *)allocation.value;
	*state = (MMapWriteState){ .parameters = parameters,
							   .file_descriptor = -1,
							   .mapped_address = NULL,
							   .adjusted_offset = 0,
							   .original_file_size = 0,
							   .file_extended = false };

	return (AsyncResult){ .poll = poll_mmap_write,
						  .state = state,
						  .status = ASYNC_PENDING,
						  .error = ERROR_RESULT_NO_ERROR };
}