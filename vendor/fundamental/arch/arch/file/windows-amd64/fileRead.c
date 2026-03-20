#include "fileRead.h"
#include "fileAdaptive.h"

AsyncResult fun_read_file_in_memory(Read parameters)
{
	FileMode mode = parameters.mode;
	if (mode == FILE_MODE_AUTO)
		mode = file_adaptive_choose(parameters.adaptive);

	if (mode == FILE_MODE_RING_BASED)
		return create_ring_read(parameters);

	MMapState *state =
		(MMapState *)fun_memory_allocate(sizeof(MMapState)).value;
	*state = (MMapState){ .parameters = parameters };

	return (AsyncResult){
		.state = state,
		.poll = poll_mmap,
		.status = ASYNC_PENDING,
	};
}
