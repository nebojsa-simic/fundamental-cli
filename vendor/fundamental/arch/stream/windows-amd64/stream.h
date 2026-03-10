#include "stream/stream.h"

#include <windows.h>

typedef struct {
  FileStream *stream; // Parent stream reference
  HANDLE file_handle; // Windows file handle
  uint64_t file_size; // Total file size
  bool file_opened;   // Initialization state
} StreamReadState;

AsyncStatus poll_stream_open(AsyncResult *result);
