#include "file/file.h"
#include "stream/stream.h"

AsyncResult fun_stream_create_file_read(String file_path, Memory buffer,
                                        uint64_t buffer_size, FileMode mode) {
  return fun_stream_open(file_path, STREAM_MODE_READ, buffer, buffer_size,
                         mode);
}

AsyncResult fun_stream_destroy(FileStream *stream) {
  if (!stream) {
    return (AsyncResult){.status = ASYNC_ERROR,
                         .error = ERROR_RESULT_NULL_POINTER};
  }
  return fun_stream_close(stream);
}
