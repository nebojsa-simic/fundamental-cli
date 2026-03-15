#ifndef LIBRARY_PROCESS_H
#define LIBRARY_PROCESS_H

#include <stddef.h>

#include "../async/async.h"
#include "../error/error.h"

/*
 * Process Module
 *
 * Asynchronous process spawning. Depends on the async module for its
 * future/await mechanism. All output memory is caller-allocated.
 *
 * Typical usage:
 *
 *   char out_buf[4096], err_buf[1024];
 *   ProcessResult proc = {
 *       .stdout_data     = out_buf,
 *       .stdout_capacity = sizeof(out_buf),
 *       .stderr_data     = err_buf,
 *       .stderr_capacity = sizeof(err_buf),
 *   };
 *
 *   AsyncResult ar = fun_process_spawn(exe, args, NULL, &proc);
 *   fun_async_await(&ar, -1);
 *   // proc.stdout_length, proc.stderr_length, proc.exit_code now valid
 *   fun_process_free(&proc);
 */

/* ------------------------------------------------------------------
 * ProcessSpawnOptions — optional spawn configuration
 * ------------------------------------------------------------------ */
typedef struct {
	const char **environment;    /* NULL-terminated env vars; NULL = inherit */
	int inherit_environment;     /* non-zero = inherit parent environment     */
} ProcessSpawnOptions;

/* ------------------------------------------------------------------
 * ProcessResult — caller-allocated output type.
 * Set stdout_data/stdout_capacity and stderr_data/stderr_capacity before
 * calling fun_process_spawn. The library fills in lengths and exit_code
 * on completion, and sets _handle immediately after spawn.
 * ------------------------------------------------------------------ */
typedef struct {
	void *_handle;           /* opaque OS handle; for terminate/free only   */
	char *stdout_data;       /* caller-provided buffer                      */
	size_t stdout_capacity;  /* size of stdout_data buffer                  */
	size_t stdout_length;    /* bytes written on completion                 */
	char *stderr_data;       /* caller-provided buffer                      */
	size_t stderr_capacity;  /* size of stderr_data buffer                  */
	size_t stderr_length;    /* bytes written on completion                 */
	int exit_code;           /* process exit code on completion             */
} ProcessResult;

/* ------------------------------------------------------------------
 * fun_process_spawn — launch a process asynchronously.
 *
 * Returns an AsyncResult in ASYNC_PENDING state immediately.
 * out->_handle is set before returning (usable for terminate/free).
 * On completion (after fun_async_await), out->stdout_length,
 * out->stderr_length, and out->exit_code are filled.
 *
 * args: NULL-terminated array; args[0] is typically the executable name.
 * options: NULL uses defaults (inherit parent environment).
 * ------------------------------------------------------------------ */
AsyncResult fun_process_spawn(const char *executable, const char **args,
                               const ProcessSpawnOptions *options,
                               ProcessResult *out);

/* ------------------------------------------------------------------
 * fun_process_terminate — forcefully kill a running process.
 * Safe to call before fun_async_await completes.
 * ------------------------------------------------------------------ */
CanReturnError(void) fun_process_terminate(ProcessResult *out);

/* ------------------------------------------------------------------
 * fun_process_free — release all OS handles associated with the process.
 * Call after fun_async_await completes (or after terminate).
 * ------------------------------------------------------------------ */
CanReturnError(void) fun_process_free(ProcessResult *out);

#endif /* LIBRARY_PROCESS_H */
