#include "fileRead.h"
#include "fileAdaptive.h"

AsyncResult fun_read_file_in_memory(Read parameters)
{
	FileMode mode = parameters.mode;
	if (mode == FILE_MODE_AUTO)
		mode = file_adaptive_choose(parameters.adaptive);

	if (mode == FILE_MODE_RING_BASED)
		return create_ring_read(parameters);

	if (!parameters.file_path || !parameters.output)
		return (AsyncResult){ .status = ASYNC_ERROR,
							  .error = ERROR_RESULT_NULL_POINTER };

	MemoryResult mem_result = fun_memory_allocate(sizeof(MMapState));
	if (fun_error_is_error(mem_result.error))
		return (AsyncResult){ .status = ASYNC_ERROR,
							  .error = mem_result.error };

	MMapState *state = (MMapState *)mem_result.value;
	*state = (MMapState){ .parameters = parameters };

	return (AsyncResult){ .state = state,
						  .poll = poll_mmap,
						  .status = ASYNC_PENDING };
}
