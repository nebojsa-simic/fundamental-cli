#include "async/async.h"
#include "error/error.h"

/* Arch-layer declaration (implemented per platform in arch/async/) */
extern unsigned long long arch_async_now_ms(void);

static void set_timeout_error(AsyncResult *result)
{
	result->status = ASYNC_ERROR;
	result->error = ERROR_RESULT_ASYNC_TIMEOUT;
}

voidResult fun_async_await(AsyncResult *result, int timeout_ms)
{
	voidResult out;
	out.error = ERROR_RESULT_NO_ERROR;

	if (timeout_ms == 0) {
		result->status = result->poll(result);
		if (result->status == ASYNC_PENDING) {
			set_timeout_error(result);
			out.error = ERROR_RESULT_ASYNC_TIMEOUT;
		} else if (result->status == ASYNC_ERROR) {
			out.error = result->error;
		}
		return out;
	}

	unsigned long long deadline = 0;
	if (timeout_ms > 0) {
		deadline = arch_async_now_ms() + (unsigned long long)timeout_ms;
	}

	while (result->status == ASYNC_PENDING) {
		result->status = result->poll(result);

		if (result->status == ASYNC_PENDING && timeout_ms > 0) {
			if (arch_async_now_ms() >= deadline) {
				set_timeout_error(result);
				out.error = ERROR_RESULT_ASYNC_TIMEOUT;
				return out;
			}
		}
	}

	if (result->status == ASYNC_ERROR) {
		out.error = result->error;
	}
	return out;
}

voidResult fun_async_await_all(AsyncResult **results, size_t count,
							   int timeout_ms)
{
	voidResult out;
	out.error = ERROR_RESULT_NO_ERROR;

	unsigned long long deadline = 0;
	if (timeout_ms > 0) {
		deadline = arch_async_now_ms() + (unsigned long long)timeout_ms;
	}

	while (1) {
		int still_pending = 0;

		for (size_t i = 0; i < count; i++) {
			if (results[i]->status == ASYNC_PENDING) {
				results[i]->status = results[i]->poll(results[i]);
				if (results[i]->status == ASYNC_PENDING) {
					still_pending = 1;
				}
			}
		}

		if (!still_pending) {
			break;
		}

		if (timeout_ms == 0) {
			/* single pass — mark all still-pending as timed out */
			for (size_t i = 0; i < count; i++) {
				if (results[i]->status == ASYNC_PENDING) {
					set_timeout_error(results[i]);
					if (out.error.code == ERROR_CODE_NO_ERROR) {
						out.error = ERROR_RESULT_ASYNC_TIMEOUT;
					}
				}
			}
			return out;
		}

		if (timeout_ms > 0 && arch_async_now_ms() >= deadline) {
			for (size_t i = 0; i < count; i++) {
				if (results[i]->status == ASYNC_PENDING) {
					set_timeout_error(results[i]);
					if (out.error.code == ERROR_CODE_NO_ERROR) {
						out.error = ERROR_RESULT_ASYNC_TIMEOUT;
					}
				}
			}
			return out;
		}
	}

	/* propagate first op error if no timeout error */
	if (out.error.code == ERROR_CODE_NO_ERROR) {
		for (size_t i = 0; i < count; i++) {
			if (results[i]->status == ASYNC_ERROR) {
				out.error = results[i]->error;
				break;
			}
		}
	}

	return out;
}
