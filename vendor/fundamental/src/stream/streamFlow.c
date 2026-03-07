#include "stream/stream.h"
#include "memory/memory.h"

bool fun_stream_can_read(FileStream *stream)
{
	return stream && stream->has_data_available && !stream->end_of_stream;
}

bool fun_stream_can_write(FileStream *stream, uint64_t requested_size)
{
	(void)stream;
	(void)requested_size;
	return false;
}

bool fun_stream_is_end_of_stream(FileStream *stream)
{
	return stream && stream->end_of_stream;
}

uint64_t fun_stream_current_position(FileStream *stream)
{
	return stream ? stream->current_position : 0;
}
