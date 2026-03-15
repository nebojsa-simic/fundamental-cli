#pragma once
#include "../error/error.h"
#include <stddef.h>

typedef enum { ASYNC_PENDING, ASYNC_COMPLETED, ASYNC_ERROR } AsyncStatus;

typedef struct AsyncResult AsyncResult;
typedef AsyncStatus (*AsyncPollFn)(AsyncResult *result);

struct AsyncResult {
	AsyncPollFn poll;
	void *state;
	AsyncStatus status;
	ErrorResult error;
};

/*
 * Wait until result completes or timeout_ms elapses.
 *   timeout_ms = -1 : block indefinitely
 *   timeout_ms =  0 : poll once and return immediately
 *   timeout_ms >  0 : wait up to timeout_ms milliseconds
 *
 * On timeout: result->status is set to ASYNC_ERROR,
 * result->error is set to ERROR_RESULT_ASYNC_TIMEOUT,
 * and that error is returned.
 */
CanReturnError(void) fun_async_await(AsyncResult *result, int timeout_ms);

/*
 * Wait until all results complete or timeout_ms elapses.
 * Timeout applies as wall-clock across all results.
 * Any result still pending at deadline is set to ASYNC_ERROR /
 * ERROR_RESULT_ASYNC_TIMEOUT. Returns the first timeout error encountered.
 */
CanReturnError(void)
	fun_async_await_all(AsyncResult **results, size_t count, int timeout_ms);
