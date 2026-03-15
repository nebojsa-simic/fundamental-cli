#include "process/process.h"
#include "error/error.h"

/* Forward declarations for arch-layer functions */
AsyncResult fun_process_arch_spawn(const char *executable, const char **args,
                                   const ProcessSpawnOptions *options,
                                   ProcessResult *out);
CanReturnError(void) fun_process_arch_terminate(ProcessResult *out);
CanReturnError(void) fun_process_arch_free(ProcessResult *out);

AsyncResult fun_process_spawn(const char *executable, const char **args,
                               const ProcessSpawnOptions *options,
                               ProcessResult *out)
{
	AsyncResult err;
	err.poll = NULL;
	err.state = NULL;
	err.status = ASYNC_ERROR;
	err.error = ERROR_RESULT_NULL_POINTER;

	if (executable == NULL || out == NULL) {
		return err;
	}

	return fun_process_arch_spawn(executable, args, options, out);
}

voidResult fun_process_terminate(ProcessResult *out)
{
	if (out == NULL) {
		voidResult r;
		r.error = ERROR_RESULT_NULL_POINTER;
		return r;
	}
	return fun_process_arch_terminate(out);
}

voidResult fun_process_free(ProcessResult *out)
{
	if (out == NULL) {
		voidResult r;
		r.error = ERROR_RESULT_NULL_POINTER;
		return r;
	}
	return fun_process_arch_free(out);
}
