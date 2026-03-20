#include "fundamental/stream/stream.h"

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct {
	FileStream *stream;
	int file_descriptor;
	uint64_t file_size;
	bool file_opened;
} StreamReadState;

AsyncStatus poll_stream_open(AsyncResult *result);