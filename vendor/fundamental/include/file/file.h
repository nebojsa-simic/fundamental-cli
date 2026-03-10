#pragma once
#include "async/async.h"
#include "error/error.h"
#include "memory/memory.h"
#include "string/string.h"
#include <stdint.h>

// ------------------------------------------------------------------
// File I/O Core Types
// ------------------------------------------------------------------

typedef enum {
  FILE_MODE_AUTO,
  FILE_MODE_STANDARD,
  FILE_MODE_MMAP,
  FILE_MODE_RING_BASED,
  FILE_MODE_DIRECT
} FileMode;

typedef struct Read {
  String file_path;       // REQUIRED - Path to file
  Memory output;          // REQUIRED - Pre-allocated buffer
  uint64_t bytes_to_read; // REQUIRED - Exact bytes to read
  uint64_t offset;        // OPTIONAL - Default 0
  FileMode mode;          // OPTIONAL - Default AUTO
} Read;

typedef struct Write {
  String file_path;        // REQUIRED - Path to file
  Memory input;            // REQUIRED - Data to write
  uint64_t bytes_to_write; // REQUIRED - Exact bytes to write
  uint64_t offset;         // OPTIONAL - Default 0
  FileMode mode;           // OPTIONAL - Default AUTO
} Write;

typedef struct Append {
  String file_path;         // REQUIRED - Path to file
  Memory input;             // REQUIRED - Data to append
  uint64_t bytes_to_append; // REQUIRED - Exact bytes to append
  FileMode mode;            // OPTIONAL - Default AUTO
} Append;

// ------------------------------------------------------------------
// Core File Operations (Explicit Required Parameters)
// ------------------------------------------------------------------

/**
 * Read exact number of bytes from file
 *
 * @param parameters {
 *   .file_path      REQUIRED - Target file path,
 *   .output         REQUIRED - Pre-allocated buffer,
 *   .bytes_to_read  REQUIRED - Must match buffer capacity,
 *   .offset         OPTIONAL - Start position (default 0),
 *   .mode           OPTIONAL - I/O strategy (default AUTO)
 * }
 *
 * @return AsyncResult with operation status
 *
 * Example:
 * fun_read_file_in_memory((Read){
 *     .file_path = path,
 *     .output = &buffer,
 *     .bytes_to_read = 2048
 * });
 */
AsyncResult fun_read_file_in_memory(Read parameters);

/**
 * Write exact number of bytes to file
 *
 * @param parameters {
 *   .file_path       REQUIRED - Target path,
 *   .input           REQUIRED - Source buffer,
 *   .bytes_to_write  REQUIRED - Must match buffer size,
 *   .offset          OPTIONAL - Write position (default 0),
 *   .mode            OPTIONAL - I/O strategy (default AUTO)
 * }
 *
 * Example:
 * fun_write_memory_to_file((Write){
 *     .file_path = path,
 *     .input = data,
 *     .bytes_to_write = data.size
 * });
 */
AsyncResult fun_write_memory_to_file(Write parameters);

/**
 * Append exact number of bytes to file
 *
 * @param parameters {
 *   .file_path        REQUIRED - Target path,
 *   .input            REQUIRED - Source buffer,
 *   .bytes_to_append  REQUIRED - Must match buffer size,
 *   .mode             OPTIONAL - I/O strategy (default AUTO)
 * }
 */
AsyncResult fun_append_memory_to_file(Append parameters);

// ------------------------------------------------------------------
// File Locking
// ------------------------------------------------------------------

/*
 * A FileLockHandle is an opaque type containing internal state that represents
 * an acquired file lock.
 */
typedef struct FileLockHandle {
  void *state; // Implementation-specific data.
} FileLockHandle;

/*
 * Lock a file for exclusive access.
 *
 * This use-case will internally open the file and acquire an exclusive lock.
 *
 * @param filePath       The path to the file to lock.
 * @param outLockHandle  Out parameter receiving an opaque lock handle.
 * @return               An ErrorResult indicating success or failure.
 */
ErrorResult fun_lock_file(String filePath, FileLockHandle *outLockHandle);

/*
 * Unlock a previously locked file.
 *
 * @param lockHandle     The lock handle acquired from fun_lock_file.
 * @return               An ErrorResult indicating success or failure.
 */
ErrorResult fun_unlock_file(FileLockHandle lockHandle);

// ------------------------------------------------------------------
// File Change Notification
// ------------------------------------------------------------------

/*
 * A callback type to be invoked when the specified file changes.
 *
 * @param filePath  The file (String) whose change is being reported.
 */
typedef void (*FileChangeCallback)(String filePath);

/*
 * Register to receive notifications when a file changes.
 *
 * The implementation will subscribe using platform-specific APIs (e.g., inotify
 * on Linux, ReadDirectoryChangesW on Windows, FSEvents on macOS, etc.) and
 * invoke the callback when a change is detected.
 *
 * @param filePath  The path to the file to monitor.
 * @param callback  The callback to invoke on changes.
 * @return          An ErrorResult indicating success or failure.
 */
AsyncResult fun_register_file_change_notification(String filePath,
                                                  FileChangeCallback callback);

/*
 * Unregister file change notifications for a given file.
 *
 * @param filePath  The file path whose notifications should be cancelled.
 * @return          An ErrorResult indicating success or failure.
 */
AsyncResult fun_unregister_file_change_notification(String filePath);
