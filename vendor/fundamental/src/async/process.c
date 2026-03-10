#include "async/async.h"

AsyncResult platform_process_spawn(AsyncResult *result, const char *executable,
                                   const char **args,
                                   const ProcessSpawnOptions *options);
AsyncStatus platform_process_poll(AsyncResult *result);
ErrorResult platform_process_terminate(Process *process);
ErrorResult platform_process_free(Process *process);

static AsyncStatus fun_process_poll(AsyncResult *result);

AsyncResult fun_async_process_spawn(const char *executable, const char **args,
                                    const ProcessSpawnOptions *options) {
  AsyncResult result;
  result.status = ASYNC_PENDING;
  result.poll = fun_process_poll;
  result.state = NULL;
  result.error = ERROR_RESULT_NO_ERROR;

  result.process.handle = NULL;
  result.process.stdout_pipe = NULL;
  result.process.stderr_pipe = NULL;
  result.process.exit_code = 0;
  result.process.stdout_write_pos = 0;
  result.process.stdout_read_pos = 0;
  result.process.stdout_count = 0;
  result.process.stderr_write_pos = 0;
  result.process.stderr_read_pos = 0;
  result.process.stderr_count = 0;
  result.process.is_running = 0;

  ProcessSpawnOptions default_options = {
      .stdout_buffer_size = PROCESS_STDOUT_BUFFER_SIZE,
      .stderr_buffer_size = PROCESS_STDERR_BUFFER_SIZE,
      .environment = NULL,
      .inherit_environment = 1,
  };

  if (options == NULL) {
    options = &default_options;
  }

  return platform_process_spawn(&result, executable, args, options);
}

Process *fun_async_result_get_process(AsyncResult *result) {
  if (result == NULL) {
    return NULL;
  }
  return &result->process;
}

const char *fun_process_get_stdout(AsyncResult *result, size_t *length) {
  if (result == NULL || length == NULL) {
    return NULL;
  }

  *length = result->process.stdout_count;
  return (const char *)result->process.stdout_buffer;
}

const char *fun_process_get_stderr(AsyncResult *result, size_t *length) {
  if (result == NULL || length == NULL) {
    return NULL;
  }

  *length = result->process.stderr_count;
  return (const char *)result->process.stderr_buffer;
}

int fun_process_get_exit_code(AsyncResult *result) {
  if (result == NULL) {
    return -1;
  }
  return result->process.exit_code;
}

ErrorResult fun_process_terminate(AsyncResult *result) {
  if (result == NULL) {
    return ERROR_RESULT_NULL_POINTER;
  }

  if (!result->process.is_running) {
    return ERROR_RESULT_NO_ERROR;
  }

  return platform_process_terminate(&result->process);
}

ErrorResult fun_process_free(AsyncResult *result) {
  if (result == NULL) {
    return ERROR_RESULT_NULL_POINTER;
  }

  return platform_process_free(&result->process);
}

static AsyncStatus fun_process_poll(AsyncResult *result) {
  if (result == NULL || result->status != ASYNC_PENDING) {
    return result ? result->status : ASYNC_ERROR;
  }

  return platform_process_poll(result);
}
