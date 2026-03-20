---
name: fundamental-async
description: Async future operations with Fundamental Library - await with timeout, poll
license: MIT
compatibility: Complements fundamental-expert skill
metadata:
  author: fundamental-library
  version: "2.0"
  category: async-operations
  related: fundamental-file-io, fundamental-process
---

# Fundamental Library - Async Operations Skill

I provide copy-paste examples for async operations using the Fundamental Library.

---

## Quick Reference

| Task | Function | Notes |
|------|----------|-------|
| Await result (indefinite) | `fun_async_await(&r, -1)` | Blocks until complete or error |
| Await with timeout | `fun_async_await(&r, 500)` | Returns error on expiry |
| Single poll | `fun_async_await(&r, 0)` | Non-blocking check |
| Await all | `fun_async_await_all(arr, n, -1)` | Wait for multiple operations |
| Check status | `r.status` | `ASYNC_PENDING / COMPLETED / ERROR` |
| Check error | `r.error.code` | `ERROR_CODE_ASYNC_TIMEOUT` on expiry |

**Status Values:**
- `ASYNC_PENDING` — Operation in progress
- `ASYNC_COMPLETED` — Operation finished successfully
- `ASYNC_ERROR` — Operation failed or timed out

---

## Task: Await Async Result

Block until an async operation completes.

```c
#include "async/async.h"
#include "file/file.h"
#include "memory/memory.h"

void async_await_example(void)
{
    MemoryResult mem_result = fun_memory_allocate(4096);
    if (fun_error_is_error(mem_result.error)) {
        return;
    }

    AsyncResult result = fun_read_file_in_memory((Read){
        .file_path = "data.txt",
        .output = mem_result.value,
        .bytes_to_read = 4096
    });

    /* Block indefinitely until complete */
    voidResult wr = fun_async_await(&result, -1);
    (void)wr;

    if (result.status == ASYNC_COMPLETED) {
        /* Success - use the data */
    } else if (result.status == ASYNC_ERROR) {
        /* Handle error */
    }

    fun_memory_free(&mem_result.value);
}
```

---

## Task: Await With Timeout

Returns `ERROR_CODE_ASYNC_TIMEOUT` and sets `status = ASYNC_ERROR` on expiry.

```c
#include "async/async.h"

void await_with_timeout_example(AsyncResult *op)
{
    /* Wait up to 500 ms */
    voidResult wr = fun_async_await(op, 500);

    if (fun_error_is_error(wr.error)) {
        if (wr.error.code == ERROR_CODE_ASYNC_TIMEOUT) {
            /* Operation did not complete in time */
        }
        return;
    }

    /* op->status == ASYNC_COMPLETED here */
}
```

**Timeout semantics:**
- `-1` — block indefinitely
- `0` — single poll, return timeout immediately if still pending
- `> 0` — deadline in milliseconds from now

---

## Task: Check Async Status

```c
#include "async/async.h"

void check_status_example(AsyncResult *result)
{
    switch (result->status) {
        case ASYNC_PENDING:
            /* Still running */
            break;
        case ASYNC_COMPLETED:
            /* Finished successfully */
            break;
        case ASYNC_ERROR:
            /* Failed — check result->error.code */
            break;
    }
}
```

---

## Task: Poll Async Operation

Non-blocking wait with periodic polling.

```c
#include "async/async.h"
#include "file/file.h"

void async_poll_example(void)
{
    AsyncResult result = fun_read_file_in_memory(params);

    while (result.status == ASYNC_PENDING) {
        /* Do other work while waiting... */

        /* Single poll — timeout=0 returns immediately */
        fun_async_await(&result, 0);
    }

    if (result.status == ASYNC_COMPLETED) {
        /* Success */
    } else {
        /* Error — result.error.code for details */
    }
}
```

---

## Task: Await Multiple Operations

```c
#include "async/async.h"

void await_all_example(void)
{
    AsyncResult r1 = async_op_1();
    AsyncResult r2 = async_op_2();

    AsyncResult *all[2] = { &r1, &r2 };
    voidResult wr = fun_async_await_all(all, 2, -1);
    (void)wr;

    /* Both r1 and r2 are now COMPLETED or ERROR */
}
```

---

## Async Error Handling

```c
AsyncResult result = some_async_operation(params);
voidResult wr = fun_async_await(&result, -1);
(void)wr;

if (result.status == ASYNC_ERROR) {
    switch (result.error.code) {
        case ERROR_CODE_ASYNC_TIMEOUT:
            /* Timed out */
            break;
        default:
            /* Other error */
            break;
    }
    return 1;
}

/* Success path */
```

---

## See Also

- **[fundamental-process.md](fundamental-process.md)** — Spawn processes, capture output
- **[fundamental-file-io.md](fundamental-file-io.md)** — Async file operations
- **[async/async.h](../../include/async/async.h)** — Complete async API
