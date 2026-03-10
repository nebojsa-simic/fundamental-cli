#include "error/error.h"
#include "file/file.h"
#include "memory/memory.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define NULL ((void *)0)

typedef long ssize_t;
typedef unsigned long size_t;
typedef long off_t;

#define SYS_write 1
#define SYS_open 2
#define SYS_close 3
#define SYS_lseek 8

#define O_WRONLY 1
#define O_CREAT 0100
#define O_APPEND 02000

#define SEEK_END 2

typedef struct {
  Append parameters;
  int file_descriptor;
  bool file_opened;
} StandardAppendState;

static inline long syscall1(long n, long a1) {
  long ret;
  __asm__ __volatile__("syscall"
                       : "=a"(ret)
                       : "a"(n), "D"(a1)
                       : "rcx", "r11", "memory");
  return ret;
}

static inline long syscall2(long n, long a1, long a2) {
  long ret;
  __asm__ __volatile__("syscall"
                       : "=a"(ret)
                       : "a"(n), "D"(a1), "S"(a2)
                       : "rcx", "r11", "memory");
  return ret;
}

static inline long syscall3(long n, long a1, long a2, long a3) {
  long ret;
  __asm__ __volatile__("syscall"
                       : "=a"(ret)
                       : "a"(n), "D"(a1), "S"(a2), "d"(a3)
                       : "rcx", "r11", "memory");
  return ret;
}

static AsyncStatus poll_standard_append(AsyncResult *result) {
  StandardAppendState *state = (StandardAppendState *)result->state;

  if (!state->file_opened) {
    int fd = (int)syscall3(SYS_open, (long)state->parameters.file_path,
                           O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd < 0) {
      result->error = fun_error_result(-fd, "Failed to open file for append");
      return ASYNC_ERROR;
    }

    state->file_descriptor = fd;
    state->file_opened = true;
    return ASYNC_PENDING;
  }

  ssize_t bytes_written = (ssize_t)syscall3(
      SYS_write, state->file_descriptor, (long)state->parameters.input,
      (long)state->parameters.bytes_to_append);
  if (bytes_written < 0) {
    result->error = fun_error_result(1, "Failed to append to file");
    syscall1(SYS_close, state->file_descriptor);
    return ASYNC_ERROR;
  }

  if ((uint64_t)bytes_written != state->parameters.bytes_to_append) {
    result->error = fun_error_result(1, "Not all bytes appended");
    syscall1(SYS_close, state->file_descriptor);
    return ASYNC_ERROR;
  }

  syscall1(SYS_close, state->file_descriptor);
  fun_memory_free((Memory *)&state);

  result->error = ERROR_RESULT_NO_ERROR;
  return ASYNC_COMPLETED;
}

static AsyncResult create_standard_append(Append parameters) {
  MemoryResult mem_result = fun_memory_allocate(sizeof(StandardAppendState));
  if (fun_error_is_error(mem_result.error)) {
    return (AsyncResult){.status = ASYNC_ERROR, .error = mem_result.error};
  }

  StandardAppendState *state = (StandardAppendState *)mem_result.value;
  *state = (StandardAppendState){
      .parameters = parameters, .file_descriptor = -1, .file_opened = false};

  return (AsyncResult){
      .state = state, .poll = poll_standard_append, .status = ASYNC_PENDING};
}

AsyncResult fun_append_memory_to_file(Append parameters) {
  switch (parameters.mode) {
  case FILE_MODE_STANDARD:
    return create_standard_append(parameters);
  case FILE_MODE_MMAP:
  case FILE_MODE_AUTO:
  default:
    return create_standard_append(parameters);
  }
}