#include "file/file.h"
#include "memory/memory.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
  Read parameters;
  int file_descriptor;
  bool file_opened;
} StandardReadState;

typedef struct {
  Read parameters;
  int file_descriptor;
  void *mapped_address;
  uint64_t adjusted_offset;
} MMapState;

AsyncStatus poll_mmap(AsyncResult *result);