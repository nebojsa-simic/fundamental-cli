#include "async/async.h"

void fun_async_await(AsyncResult *result)
{
	// Continuously poll until the operation is no longer pending.
	while (result->status == ASYNC_PENDING) {
		// Call the poll function to update the status.
		AsyncStatus new_status = result->poll(result);
		result->status = new_status;
	}
}

void fun_async_await_all(AsyncResult **results, size_t count)
{
	// Loop until every async result is no longer pending.
	while (1) {
		bool isStillPending = false;
		for (size_t i = 0; i < count; i++) {
			if (results[i]->status == ASYNC_PENDING) {
				// Poll the pending async operation.
				results[i]->status = results[i]->poll(results[i]);

				// Mark if at least one operation is still pending.
				if (results[i]->status == ASYNC_PENDING) {
					isStillPending = true;
				}
			}
		}

		// Exit once all operations are complete or have errored.
		if (!isStillPending) {
			break;
		}

		// TODD: platform-appropriate yield or sleep to avoid busy-waiting.
	}
}
