---
name: fundamental-async
description: Async operations with Fundamental Library - await, poll, process spawn
license: MIT
compatibility: Complements fundamental-expert skill
metadata:
  author: fundamental-library
  version: "1.0"
  category: async-operations
  related: fundamental-file-io, fundamental-process-spawn
---

# Fundamental Library - Async Operations Skill

I provide copy-paste examples for async operations using the Fundamental Library.

---

## Quick Reference

| Task | Function | Example |
|------|----------|---------|
| Await result | `fun_async_await()` | See below |
| Poll status | `fun_async_poll()` | See below |
| Check status | Check `.status` field | See below |
| Spawn process | `fun_async_process_spawn()` | See below |
| Get exit code | `fun_process_get_exit_code()` | See below |

**Status Values:**
- `ASYNC_PENDING` - Operation in progress
- `ASYNC_COMPLETED` - Operation finished successfully
- `ASYNC_ERROR` - Operation failed

---

## Task: Await Async Result

Block until an async operation completes.

```c
#include "async/async.h"
#include "file/file.h"
#include "memory/memory.h"

void async_await_example(void)
{
    // Allocate buffer
    MemoryResult mem_result = fun_memory_allocate(4096);
    if (fun_error_is_error(mem_result.error)) {
        return;
    }
    
    // Start async operation
    AsyncResult result = fun_read_file_in_memory((Read){
        .file_path = "data.txt",
        .output = mem_result.value,
        .bytes_to_read = 4096
    });
    
    // Block until complete
    fun_async_await(&result);
    
    // Check status
    if (result.status == ASYNC_COMPLETED) {
        // Success - use the data
    } else if (result.status == ASYNC_ERROR) {
        // Handle error
    }
    
    fun_memory_free(&mem_result.value);
}
```

**Key Points:**
- `fun_async_await()` blocks the current thread
- Use for simple sequential code
- For non-blocking, use poll pattern instead

---

## Task: Check Async Status

Check the status of an async operation without blocking.

```c
#include "async/async.h"

void check_status_example(AsyncResult *result)
{
    switch (result->status) {
        case ASYNC_PENDING:
            // Still running
            break;
        case ASYNC_COMPLETED:
            // Finished successfully
            break;
        case ASYNC_ERROR:
            // Failed - check result.error
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
    // Start async operation
    AsyncResult result = fun_read_file_in_memory(params);
    
    // Poll until complete
    while (result.status == ASYNC_PENDING) {
        // Do other work while waiting...
        
        // Poll the operation
        fun_async_poll(&result);
        
        // Optional: yield to other tasks
        // fun_yield();
    }
    
    // Now complete or error
    if (result.status == ASYNC_COMPLETED) {
        // Success
    } else {
        // Error
    }
}
```

**Key Points:**
- Non-blocking - you can do other work in the loop
- Call `fun_async_poll()` to check progress
- Use for UI applications or concurrent operations

---

## Task: Spawn Process

Execute an external program.

```c
#include "async/async.h"
#include "console/console.h"

void process_spawn_example(void)
{
    // Set up process options
    ProcessSpawnOptions options = {
        .executable = "ls",
        .arguments = (const char*[]){"ls", "-la", NULL},
        .capture_stdout = 1,
        .capture_stderr = 0,
        .inherit_environment = 1
    };
    
    // Spawn process
    AsyncResult spawn_result = fun_async_process_spawn(&options);
    
    // Wait for completion
    fun_async_await(&spawn_result);
    
    // Check for spawn errors
    if (spawn_result.status == ASYNC_ERROR) {
        fun_console_error_line("Failed to spawn process");
        return;
    }
    
    // Get exit code
    int exit_code = fun_process_get_exit_code(&spawn_result);
    
    if (exit_code == 0) {
        fun_console_write_line("Process completed successfully");
    } else {
        fun_console_write_line("Process exited with code: ");
        // ... print exit_code ...
    }
    
    // Get captured stdout (if capture_stdout was set)
    Memory stdout = fun_process_get_stdout(&spawn_result);
    if (stdout != NULL) {
        // Process stdout...
    }
    
    // Cleanup
    fun_process_free(&spawn_result);
}
```

---

## Task: Capture Process Output

Read stdout from a spawned process.

```c
#include "async/async.h"
#include "memory/memory.h"
#include "console/console.h"

void capture_output_example(void)
{
    ProcessSpawnOptions options = {
        .executable = "echo",
        .arguments = (const char*[]){"echo", "Hello from process", NULL},
        .capture_stdout = 1,
        .inherit_environment = 1
    };
    
    AsyncResult result = fun_async_process_spawn(&options);
    fun_async_await(&result);
    
    if (result.status == ASYNC_COMPLETED) {
        // Get output buffer
        Memory output = fun_process_get_stdout(&result);
        
        if (output != NULL) {
            // Output contains the captured stdout
            fun_console_write("Process output: ");
            fun_console_write((const char *)output);
        }
    }
    
    fun_process_free(&result);
}
```

---

## Async Error Handling

**Pattern for all async operations:**

```c
AsyncResult result = some_async_operation(params);
fun_async_await(&result);  // Or poll in a loop

if (result.status == ASYNC_ERROR) {
    // Check error code
    switch (result.error.code) {
        case ERROR_CODE_PROCESS_SPAWN_FAILED:
            // Process couldn't start
            break;
        case ERROR_CODE_PROCESS_NOT_FOUND:
            // Executable not found
            break;
        default:
            // Other error
            break;
    }
    return 1;
}

// Success path
// ... use result ...
```

---

## Common Patterns

### Fire and Forget (with error check)
```c
AsyncResult r = fun_async_operation(params);
fun_async_await(&r);

if (r.status == ASYNC_ERROR) {
    // Handle error
    return;
}
// Success - continue
```

### Multiple Concurrent Operations
```c
AsyncResult r1 = async_op_1();
AsyncResult r2 = async_op_2();
AsyncResult r3 = async_op_3();

// Poll all until done
while (r1.status == ASYNC_PENDING || 
       r2.status == ASYNC_PENDING || 
       r3.status == ASYNC_PENDING) {
    
    fun_async_poll(&r1);
    fun_async_poll(&r2);
    fun_async_poll(&r3);
    
    // Do other work...
}
```

### Timeout Pattern
```c
AsyncResult result = async_operation();

// Poll with timeout (simplified)
int iterations = 0;
int max_iterations = 1000;  // ~10 seconds if polling every 10ms

while (result.status == ASYNC_PENDING && iterations < max_iterations) {
    fun_async_poll(&result);
    iterations++;
    // fun_sleep(10);  // If available
}

if (result.status == ASYNC_PENDING) {
    // Timeout - handle accordingly
}
```

---

## See Also

- **[fundamental-file-io.md](fundamental-file-io.md)** - Async file operations
- **[async/async.h](../../include/async/async.h)** - Complete async API
- **[process_spawn tests](../../tests/process_spawn/)** - Process spawn examples
