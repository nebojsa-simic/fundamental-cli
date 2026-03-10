#include "memory/memory.h"
#include "stream/stream.h"

bool fun_stream_can_read(FileStream *stream) {
  return stream && stream->has_data_available && !stream->end_of_stream;
}

bool fun_stream_can_write(FileStream *stream, uint64_t requested_size) {
  if (!stream) {
    return false;
  }

  // Check if stream is in write or append mode
  if (stream->mode != STREAM_MODE_WRITE && stream->mode != STREAM_MODE_APPEND) {
    return false;
  }

  // Check if stream is at end (error state or closed)
  if (stream->end_of_stream) {
    return false;
  }

  // For now, assume we can always write to a valid write stream
  // The actual write operation will handle disk space / permission errors
  (void)requested_size;
  return true;
}

bool fun_stream_is_end_of_stream(FileStream *stream) {
  return stream && stream->end_of_stream;
}

uint64_t fun_stream_current_position(FileStream *stream) {
  return stream ? stream->current_position : 0;
}
