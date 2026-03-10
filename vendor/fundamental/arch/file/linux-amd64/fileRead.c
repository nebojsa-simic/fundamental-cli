#include "fileRead.h"

#include <stddef.h>
#include <stdint.h>

#define NULL ((void *)0)

typedef long ssize_t;
typedef unsigned long size_t;
typedef long off_t;

#define SYS_read 0
#define SYS_open 2
#define SYS_close 3
#define SYS_fstat 5
#define SYS_lseek 8

#define O_RDONLY 0
#define O_WRONLY 1
#define O_RDWR 2
#define O_CREAT 0100
#define O_TRUNC 01000

#define SEEK_SET 0

struct stat {
  unsigned long st_dev;
  unsigned long st_ino;
  unsigned long st_nlink;
  unsigned int st_mode;
  unsigned int st_uid;
  unsigned int st_gid;
  unsigned long st_rdev;
  unsigned long st_size;
  unsigned long st_blksize;
  unsigned long st_blocks;
  unsigned long st_atime;
  unsigned long st_atime_nsec;
  unsigned long st_mtime;
  unsigned long st_mtime_nsec;
  unsigned long st_ctime;
  unsigned long st_ctime_nsec;
  unsigned long __unused[3];
};

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

static AsyncStatus poll_standard_read(AsyncResult *result) {
  StandardReadState *state = (StandardReadState *)result->state;

  if (!state->file_opened) {
    int fd =
        (int)syscall2(SYS_open, (long)state->parameters.file_path, O_RDONLY);
    if (fd < 0) {
      result->error = fun_error_result(-fd, "Failed to open file");
      return ASYNC_ERROR;
    }

    state->file_descriptor = fd;
    state->file_opened = true;
    return ASYNC_PENDING;
  }

  off_t seek_result = (off_t)syscall3(SYS_lseek, state->file_descriptor,
                                      (long)state->parameters.offset, SEEK_SET);
  if ((long)seek_result < 0) {
    result->error = fun_error_result(1, "Failed to seek in file");
    goto cleanup;
  }

  ssize_t bytes_read = (ssize_t)syscall3(SYS_read, state->file_descriptor,
                                         (long)state->parameters.output,
                                         (long)state->parameters.bytes_to_read);
  if (bytes_read < 0) {
    result->error = fun_error_result(1, "Failed to read file");
    goto cleanup;
  }

  syscall1(SYS_close, state->file_descriptor);
  state->file_descriptor = -1;
  fun_memory_free((Memory *)&state);
  result->state = NULL;

  result->error = ERROR_RESULT_NO_ERROR;
  return ASYNC_COMPLETED;

cleanup:
  if (state) {
    if (state->file_descriptor >= 0) {
      syscall1(SYS_close, state->file_descriptor);
    }
    fun_memory_free((Memory *)&state);
    result->state = NULL;
  }
  return ASYNC_ERROR;
}

static AsyncResult create_standard_read(Read parameters) {
  if (!parameters.file_path || !parameters.output) {
    return (AsyncResult){.status = ASYNC_ERROR,
                         .error = ERROR_RESULT_NULL_POINTER};
  }

  MemoryResult mem_result = fun_memory_allocate(sizeof(StandardReadState));
  if (fun_error_is_error(mem_result.error)) {
    return (AsyncResult){.status = ASYNC_ERROR, .error = mem_result.error};
  }

  StandardReadState *state = (StandardReadState *)mem_result.value;
  *state = (StandardReadState){
      .parameters = parameters, .file_descriptor = -1, .file_opened = false};

  return (AsyncResult){
      .state = state, .poll = poll_standard_read, .status = ASYNC_PENDING};
}

AsyncResult fun_read_file_in_memory(Read parameters) {
  switch (parameters.mode) {
  case FILE_MODE_STANDARD:
    return create_standard_read(parameters);
  case FILE_MODE_MMAP:
  case FILE_MODE_AUTO:
  default: {
    MemoryResult mem_result = fun_memory_allocate(sizeof(MMapState));
    if (fun_error_is_error(mem_result.error)) {
      return (AsyncResult){.status = ASYNC_ERROR, .error = mem_result.error};
    }

    MMapState *state = (MMapState *)mem_result.value;
    *state = (MMapState){.parameters = parameters};

    return (AsyncResult){
        .state = state, .poll = poll_mmap, .status = ASYNC_PENDING};
  }
  }
}