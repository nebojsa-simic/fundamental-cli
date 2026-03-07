#include "fileWrite.h"

AsyncResult fun_write_memory_to_file(Write parameters)
{
	// Validate required parameters
	if (!parameters.file_path || !parameters.input) {
		return (AsyncResult){ .status = ASYNC_ERROR,
							  .error = ERROR_RESULT_NULL_POINTER };
	}

	// Handle zero-byte writes as early success
	if (parameters.bytes_to_write == 0) {
		return (AsyncResult){ .status = ASYNC_COMPLETED,
							  .error = ERROR_RESULT_NO_ERROR };
	}

	// Allocate state structure
	MemoryResult allocation = fun_memory_allocate(sizeof(MMapWriteState));
	if (fun_error_is_error(allocation.error)) {
		return (AsyncResult){ .status = ASYNC_ERROR,
							  .error = allocation.error };
	}

	MMapWriteState *state = (MMapWriteState *)allocation.value;
	*state = (MMapWriteState){ .parameters = parameters,
							   .file_handle = INVALID_HANDLE_VALUE,
							   .mapping_handle = NULL,
							   .mapped_view = NULL,
							   .file_extended = false };

	return (AsyncResult){ .poll = poll_mmap_write,
						  .state = state,
						  .status = ASYNC_PENDING,
						  .error = ERROR_RESULT_NO_ERROR };
}
