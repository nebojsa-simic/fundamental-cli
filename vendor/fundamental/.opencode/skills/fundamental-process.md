---
name: fundamental-process
description: Process spawn, terminate, and output capture with Fundamental Library
license: MIT
compatibility: Complements fundamental-expert skill
metadata:
  author: fundamental-library
  version: "1.0"
  category: process
  related: fundamental-async
---

# Fundamental Library - Process Skill

I provide copy-paste examples for process operations using the Fundamental Library.

---

## Quick Reference

| Task | Function | Notes |
|------|----------|-------|
| Spawn process | `fun_process_spawn(exe, args, opts, &proc)` | Returns `AsyncResult` |
| Await completion | `fun_async_await(&ar, -1)` | Block until done |
| Read stdout | `proc.stdout_data` / `proc.stdout_length` | Direct field access |
| Read stderr | `proc.stderr_data` / `proc.stderr_length` | Direct field access |
| Read exit code | `proc.exit_code` | Set after completion |
| Terminate | `fun_process_terminate(&proc)` | Send SIGKILL / TerminateProcess |
| Free resources | `fun_process_free(&proc)` | Close handles, pipes |

**Caller allocates buffers** — pass `stdout_data`/`stderr_data` pointers and capacities before spawning.

---

## Task: Spawn Process and Capture Output

```c
#include "async/async.h"
#include "process/process.h"

void spawn_and_capture(void)
{
    char out_buf[4096], err_buf[1024];
    ProcessResult proc = {
        .stdout_data     = out_buf,
        .stdout_capacity = sizeof(out_buf),
        .stderr_data     = err_buf,
        .stderr_capacity = sizeof(err_buf)
    };

#ifdef _WIN32
    const char *args[] = { "cmd.exe", "/c", "echo", "Hello", NULL };
    AsyncResult ar = fun_process_spawn("cmd.exe", args, NULL, &proc);
#else
    const char *args[] = { "/bin/sh", "-c", "echo Hello", NULL };
    AsyncResult ar = fun_process_spawn("/bin/sh", args, NULL, &proc);
#endif

    if (ar.error.code != ERROR_CODE_NO_ERROR) {
        /* Spawn failed — check ar.error.code */
        return;
    }

    fun_async_await(&ar, -1);

    if (ar.status == ASYNC_COMPLETED) {
        /* proc.stdout_data contains output, proc.stdout_length bytes */
        /* proc.exit_code contains the exit status */
    }

    fun_process_free(&proc);
}
```

---

## Task: Check Exit Code

```c
void check_exit_code(void)
{
    char out_buf[256], err_buf[256];
    ProcessResult proc = {
        .stdout_data     = out_buf,
        .stdout_capacity = sizeof(out_buf),
        .stderr_data     = err_buf,
        .stderr_capacity = sizeof(err_buf)
    };

    const char *args[] = { "/bin/sh", "-c", "exit 42", NULL };
    AsyncResult ar = fun_process_spawn("/bin/sh", args, NULL, &proc);

    if (ar.error.code == ERROR_CODE_NO_ERROR) {
        fun_async_await(&ar, -1);
        /* proc.exit_code == 42 */
    }

    fun_process_free(&proc);
}
```

---

## Task: Handle Process Not Found

```c
void handle_not_found(void)
{
    char out_buf[256], err_buf[256];
    ProcessResult proc = {
        .stdout_data     = out_buf,
        .stdout_capacity = sizeof(out_buf),
        .stderr_data     = err_buf,
        .stderr_capacity = sizeof(err_buf)
    };

    const char *args[] = { "nonexistent_tool", NULL };
    AsyncResult ar = fun_process_spawn("nonexistent_tool", args, NULL, &proc);

    if (ar.error.code == ERROR_CODE_PROCESS_NOT_FOUND) {
        /* Executable not found — ar is already in error state, no await needed */
        return;
    }

    fun_async_await(&ar, -1);
    fun_process_free(&proc);
}
```

---

## Task: Terminate a Running Process

```c
void terminate_example(void)
{
    char out_buf[256], err_buf[256];
    ProcessResult proc = {
        .stdout_data     = out_buf,
        .stdout_capacity = sizeof(out_buf),
        .stderr_data     = err_buf,
        .stderr_capacity = sizeof(err_buf)
    };

    const char *args[] = { "/bin/sh", "-c", "sleep 60", NULL };
    AsyncResult ar = fun_process_spawn("/bin/sh", args, NULL, &proc);

    if (ar.error.code == ERROR_CODE_NO_ERROR) {
        voidResult term = fun_process_terminate(&proc);
        (void)term;
        fun_async_await(&ar, -1);
    }

    fun_process_free(&proc);
}
```

---

## Task: Capture stderr

```c
void capture_stderr_example(void)
{
    char out_buf[256], err_buf[4096];
    ProcessResult proc = {
        .stdout_data     = out_buf,
        .stdout_capacity = sizeof(out_buf),
        .stderr_data     = err_buf,
        .stderr_capacity = sizeof(err_buf)
    };

    const char *args[] = { "/bin/sh", "-c", "ls /nonexistent", NULL };
    AsyncResult ar = fun_process_spawn("/bin/sh", args, NULL, &proc);

    if (ar.error.code == ERROR_CODE_NO_ERROR) {
        fun_async_await(&ar, -1);
        /* proc.stderr_data contains error output, proc.stderr_length bytes */
    }

    fun_process_free(&proc);
}
```

---

## Error Codes

| Code | Meaning |
|------|---------|
| `ERROR_CODE_NO_ERROR` | Spawn succeeded |
| `ERROR_CODE_PROCESS_NOT_FOUND` | Executable not found |
| `ERROR_CODE_PROCESS_SPAWN_FAILED` | OS-level spawn failure |
| `ERROR_CODE_PROCESS_TERMINATE_FAILED` | Terminate call failed |

---

## See Also

- **[fundamental-async.md](fundamental-async.md)** — Await, timeout, poll
- **[process/process.h](../../include/process/process.h)** — Complete process API
- **[tests/process/](../../tests/process/)** — Process test examples
