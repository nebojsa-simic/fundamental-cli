#pragma once
#include "../error/error.h"
#include <stddef.h>
#include <stdint.h>

typedef enum { ASYNC_PENDING, ASYNC_COMPLETED, ASYNC_ERROR } AsyncStatus;

typedef struct AsyncResult AsyncResult;
typedef AsyncStatus (*AsyncPollFn)(AsyncResult *result);

#define PROCESS_STDOUT_BUFFER_SIZE 4096
#define PROCESS_STDERR_BUFFER_SIZE 4096

typedef struct {
	void *handle;
	void *stdout_pipe;
	void *stderr_pipe;
	int exit_code;
	uint8_t stdout_buffer[PROCESS_STDOUT_BUFFER_SIZE];
	size_t stdout_write_pos;
	size_t stdout_read_pos;
	size_t stdout_count;
	uint8_t stderr_buffer[PROCESS_STDERR_BUFFER_SIZE];
	size_t stderr_write_pos;
	size_t stderr_read_pos;
	size_t stderr_count;
	int is_running;
	int platform_data[16];
} Process;

typedef struct {
	size_t stdout_buffer_size;
	size_t stderr_buffer_size;
	const char **environment;
	int inherit_environment;
} ProcessSpawnOptions;

struct AsyncResult {
	AsyncPollFn poll;
	void *state;
	AsyncStatus status;
	ErrorResult error;
	Process process;
};

void fun_async_await(AsyncResult *result);
void fun_async_await_all(AsyncResult **results, size_t count);

/**
 * Spawn a new process asynchronously.
 * @param executable Path to the executable to run
 * @param args NULL-terminated array of arguments (args[0] is typically the executable)
 * @param options Spawn options, or NULL for defaults
 * @return AsyncResult with process handle embedded. Use fun_async_await() to wait.
 */
AsyncResult fun_async_process_spawn(const char *executable, const char **args,
									const ProcessSpawnOptions *options);

/**
 * Get the Process struct embedded in an AsyncResult from process spawn.
 * @param result AsyncResult from fun_async_process_spawn()
 * @return Pointer to Process struct, or NULL if result is NULL
 */
Process *fun_async_result_get_process(AsyncResult *result);

/**
 * Get captured stdout from a spawned process.
 * @param result AsyncResult from fun_async_process_spawn()
 * @param length Output parameter for stdout length
 * @return Pointer to stdout buffer, or NULL if invalid
 */
const char *fun_process_get_stdout(AsyncResult *result, size_t *length);

/**
 * Get captured stderr from a spawned process.
 * @param result AsyncResult from fun_async_process_spawn()
 * @param length Output parameter for stderr length
 * @return Pointer to stderr buffer, or NULL if invalid
 */
const char *fun_process_get_stderr(AsyncResult *result, size_t *length);

/**
 * Get the exit code of a completed process.
 * @param result AsyncResult from fun_async_process_spawn()
 * @return Exit code, or -1 if invalid
 */
int fun_process_get_exit_code(AsyncResult *result);

/**
 * Forcefully terminate a running process.
 * @param result AsyncResult from fun_async_process_spawn()
 * @return ErrorResult indicating success or failure
 */
ErrorResult fun_process_terminate(AsyncResult *result);

/**
 * Release resources associated with a spawned process.
 * @param result AsyncResult from fun_async_process_spawn()
 * @return ErrorResult indicating success or failure
 */
ErrorResult fun_process_free(AsyncResult *result);
