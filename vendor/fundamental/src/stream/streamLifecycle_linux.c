#include "memory/memory.h"
#include "stream/stream.h"
#include <stddef.h>
#include <stdint.h>

typedef struct {
  FileStream *stream;
  int file_descriptor;
  uint64_t file_size;
  int file_opened;
} StreamReadState;

AsyncResult fun_stream_close(FileStream *stream) {
  if (!stream) {
    return (AsyncResult){.status = ASYNC_ERROR,
                         .error = ERROR_RESULT_NULL_POINTER};
  }

  StreamReadState *state = (StreamReadState *)stream->internal_state;
  if (state && state->file_descriptor >= 0) {
    long ret;
    __asm__ __volatile__("syscall"
                         : "=a"(ret)
                         : "a"(3), "D"(state->file_descriptor)
                         : "rcx", "r11", "memory");
  }
  if (state) {
    fun_memory_free((Memory *)&state);
  }

  fun_memory_free((Memory *)&stream);
  return (AsyncResult){.status = ASYNC_COMPLETED,
                       .error = ERROR_RESULT_NO_ERROR};
}
