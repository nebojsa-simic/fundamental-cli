#pragma once
#include <stdint.h>
#include "../memory/memory.h"
#include "../string/string.h"
#include "../error/error.h"
#include "../async/async.h"

// ------------------------------------------------------------------
// File I/O Core Types
// ------------------------------------------------------------------

typedef enum {
	FILE_MODE_AUTO,
	FILE_MODE_MMAP,
	FILE_MODE_RING_BASED,
} FileMode;

// Per-handle EMA state for adaptive mode switching.
// Zero-initialise before first use; pass NULL to disable tracking.
typedef struct {
	float iops_ema; // exponential moving average of ops/sec
	float bytes_ema; // exponential moving average of bytes/sec
	uint64_t last_op_ns; // monotonic timestamp of last completed op (ns)
} FileAdaptiveState;

typedef struct Read {
	String file_path; // REQUIRED - Path to file
	Memory output; // REQUIRED - Pre-allocated buffer
	uint64_t bytes_to_read; // REQUIRED - Exact bytes to read
	uint64_t offset; // OPTIONAL - Default 0
	FileMode mode; // OPTIONAL - Default AUTO
	FileAdaptiveState *adaptive; // OPTIONAL - Pass to enable adaptive switching
} Read;

typedef struct Write {
	String file_path; // REQUIRED - Path to file
	Memory input; // REQUIRED - Data to write
	uint64_t bytes_to_write; // REQUIRED - Exact bytes to write
	uint64_t offset; // OPTIONAL - Default 0
	FileMode mode; // OPTIONAL - Default AUTO
	FileAdaptiveState *adaptive; // OPTIONAL - Pass to enable adaptive switching
} Write;

typedef struct Append {
	String file_path; // REQUIRED - Path to file
	Memory input; // REQUIRED - Data to append
	uint64_t bytes_to_append; // REQUIRED - Exact bytes to append
	FileMode mode; // OPTIONAL - Default AUTO
	FileAdaptiveState *adaptive; // OPTIONAL - Pass to enable adaptive switching
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
 *   .mode           OPTIONAL - I/O strategy (default AUTO),
 *   .adaptive       OPTIONAL - EMA state for adaptive switching
 * }
 *
 * @return AsyncResult with operation status
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
 *   .mode            OPTIONAL - I/O strategy (default AUTO),
 *   .adaptive        OPTIONAL - EMA state for adaptive switching
 * }
 */
AsyncResult fun_write_memory_to_file(Write parameters);

/**
 * Append exact number of bytes to file
 *
 * @param parameters {
 *   .file_path        REQUIRED - Target path,
 *   .input            REQUIRED - Source buffer,
 *   .bytes_to_append  REQUIRED - Must match buffer size,
 *   .mode             OPTIONAL - I/O strategy (default AUTO),
 *   .adaptive         OPTIONAL - EMA state for adaptive switching
 * }
 */
AsyncResult fun_append_memory_to_file(Append parameters);

// ------------------------------------------------------------------
// File Locking
// ------------------------------------------------------------------

/*
 * A FileLockHandle is an opaque type containing internal state that represents an acquired file lock.
 */
typedef struct FileLockHandle {
	void *state; // Implementation-specific data.
} FileLockHandle;

/*
 * Lock a file for exclusive access.
 */
ErrorResult fun_lock_file(String filePath, FileLockHandle *outLockHandle);

/*
 * Unlock a previously locked file.
 */
ErrorResult fun_unlock_file(FileLockHandle lockHandle);

// ------------------------------------------------------------------
// File Change Notification
// ------------------------------------------------------------------

typedef void (*FileChangeCallback)(String filePath);

AsyncResult fun_register_file_change_notification(String filePath,
												  FileChangeCallback callback);

AsyncResult fun_unregister_file_change_notification(String filePath);
