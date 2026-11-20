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

/*
 * Controls how writes are flushed to storage.
 *
 * FILE_DURABILITY_ASYNC  - Default. Data lands in page cache; fastest but
 *                          may be lost on crash before OS flushes to disk.
 * FILE_DURABILITY_SYNC   - Calls msync(MS_SYNC) after mmap write, or
 *                          fsync() after append. Data on disk when call
 *                          returns; moderate overhead.
 * FILE_DURABILITY_FULL   - Calls fsync() after write. Data and metadata
 *                          guaranteed on disk; slowest but safest.
 */
typedef enum {
	FILE_DURABILITY_ASYNC = 0, /* default - page cache only */
	FILE_DURABILITY_SYNC, /* msync/fsync after write */
	FILE_DURABILITY_FULL, /* fsync after write (data + metadata) */
} FileDurabilityMode;

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
	FileDurabilityMode durability_mode; // OPTIONAL - Default ASYNC
	FileAdaptiveState *adaptive; // OPTIONAL - Pass to enable adaptive switching
} Write;

typedef struct Append {
	String file_path; // REQUIRED - Path to file
	Memory input; // REQUIRED - Data to append
	uint64_t bytes_to_append; // REQUIRED - Exact bytes to append
	FileMode mode; // OPTIONAL - Default AUTO
	FileDurabilityMode durability_mode; // OPTIONAL - Default ASYNC
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
 *   .file_path        REQUIRED - Target path,
 *   .input            REQUIRED - Source buffer,
 *   .bytes_to_write   REQUIRED - Must match buffer size,
 *   .offset           OPTIONAL - Write position (default 0),
 *   .mode             OPTIONAL - I/O strategy (default AUTO),
 *   .durability_mode  OPTIONAL - Flush guarantee (default ASYNC):
 *                       FILE_DURABILITY_ASYNC - page cache only, fastest
 *                       FILE_DURABILITY_SYNC  - msync(MS_SYNC) after mmap write
 *                       FILE_DURABILITY_FULL  - fsync() after write
 *   .adaptive         OPTIONAL - EMA state for adaptive switching
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
 *   .durability_mode  OPTIONAL - Flush guarantee (default ASYNC):
 *                       FILE_DURABILITY_ASYNC - page cache only, fastest
 *                       FILE_DURABILITY_SYNC  - fsync() after append
 *                       FILE_DURABILITY_FULL  - fsync() after append
 *   .adaptive         OPTIONAL - EMA state for adaptive switching
 * }
 */
AsyncResult fun_append_memory_to_file(Append parameters);

// ------------------------------------------------------------------
// File Locking
// ------------------------------------------------------------------

/*
 * Opaque handle representing an acquired file lock.
 * Initialised by fun_lock_file() / fun_file_lock_with_timeout().
 * Must be released with fun_unlock_file().
 */
typedef struct FileLockHandle {
	void *state; /* implementation-specific data; NULL means not locked */
} FileLockHandle;

/*
 * Acquire an exclusive lock on a file.
 *
 * Equivalent to fun_file_lock_with_timeout(filePath, 5000, outLockHandle).
 * Retries every 100 ms up to the 5-second default.
 *
 * @param filePath      Path of the file to lock (must already exist).
 * @param outLockHandle Receives the lock handle on success.
 * @return              OK on success; ERROR_CODE_LOCK_TIMEOUT if no lock
 *                      could be acquired within 5 seconds; other error codes
 *                      on system failures.
 */
ErrorResult fun_lock_file(String filePath, FileLockHandle *outLockHandle);

/*
 * Acquire an exclusive lock on a file with an explicit timeout.
 *
 * Uses LOCK_EX | LOCK_NB with a monotonic-clock retry loop.  Each retry
 * sleeps ~100 ms before re-attempting.  If timeout_ms elapses without
 * acquiring the lock the function returns ERROR_CODE_LOCK_TIMEOUT (15).
 *
 * @param filePath      Path of the file to lock (must already exist).
 * @param timeout_ms    Maximum wait in milliseconds (0 = one attempt only).
 * @param outLockHandle Receives the lock handle on success.
 * @return              OK on success; ERROR_CODE_LOCK_TIMEOUT on timeout;
 *                      other error codes on system failures.
 */
ErrorResult fun_file_lock_with_timeout(String filePath, uint32_t timeout_ms,
									   FileLockHandle *outLockHandle);

/*
 * Release a previously acquired file lock.
 *
 * The lock handle is invalidated after this call.  Passing a handle whose
 * .state is NULL returns an error without crashing.
 */
ErrorResult fun_unlock_file(FileLockHandle lockHandle);

// ------------------------------------------------------------------
// File Change Notification
// ------------------------------------------------------------------

typedef void (*FileChangeCallback)(String filePath);

/*
 * Register a callback to fire whenever the file at filePath changes.
 *
 * Returns an AsyncResult whose .state must be passed to
 * fun_unregister_file_change_notification() to clean up resources.
 */
AsyncResult fun_register_file_change_notification(String filePath,
												  FileChangeCallback callback);

/*
 * Unregister a file change notification.
 *
 * MIGRATION NOTE (harden-file-module): the old signature accepted a
 * String filePath; the new signature accepts the opaque void *state
 * returned in AsyncResult.state by fun_register_file_change_notification().
 *
 *   Old (removed):
 *     fun_unregister_file_change_notification(filePath);
 *   New:
 *     AsyncResult reg = fun_register_file_change_notification(...);
 *     fun_unregister_file_change_notification(reg.state);
 *
 * Passing NULL returns an error without crashing.
 */
AsyncResult fun_unregister_file_change_notification(void *state);
