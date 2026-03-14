---
name: fundamental-file-io
description: File I/O operations with Fundamental Library - read, write, append, and check file existence
license: MIT
compatibility: Complements fundamental-expert skill
metadata:
  author: fundamental-library
  version: "1.0"
  category: file-operations
  related: fundamental-memory, fundamental-directory, fundamental-stream
---

# Fundamental Library - File I/O Skill

I provide copy-paste examples for file operations using the Fundamental Library.

---

## Quick Reference

| Task | Function | Example |
|------|----------|---------|
| Read file | `fun_read_file_in_memory()` | See below |
| Write file | `fun_write_memory_to_file()` | See below |
| Append file | `fun_append_memory_to_file()` | See below |
| Check exists | `fun_file_exists()` | See below |
| Stream read | `fun_stream_create_file_read()` | See fundamental-stream |

**See Also:** [fundamental-memory](fundamental-memory.md) for allocation patterns, [fundamental-directory](fundamental-directory.md) for directory operations

---

## Task: Read a File

Read entire file contents into a pre-allocated buffer.

```c
#include "file/file.h"
#include "memory/memory.h"
#include "console/console.h"

int read_file_example(void)
{
    // STEP 1: Allocate buffer for file contents
    MemoryResult mem_result = fun_memory_allocate(4096);
    if (fun_error_is_error(mem_result.error)) {
        fun_console_error_line("Failed to allocate memory");
        return 1;
    }
    Memory buffer = mem_result.value;
    
    // STEP 2: Read file asynchronously
    AsyncResult read_result = fun_read_file_in_memory((Read){
        .file_path = "path/to/file.txt",
        .output = buffer,
        .bytes_to_read = 4096,
        .offset = 0
    });
    
    // STEP 3: Wait for completion
    fun_async_await(&read_result);
    
    // STEP 4: Check for errors
    if (read_result.status == ASYNC_ERROR) {
        fun_console_error_line("Failed to read file");
        fun_memory_free(&buffer);
        return 1;
    }
    
    // STEP 5: Use file contents
    // buffer now contains the file data
    fun_console_write_line("File read successfully");
    
    // STEP 6: Cleanup
    voidResult free_result = fun_memory_free(&buffer);
    
    return 0;
}
```

**Key Points:**
- Buffer must be pre-allocated (caller-allocated memory pattern)
- Async operation requires `fun_async_await()` to block until complete
- Always check `read_result.status` for `ASYNC_ERROR`
- Free memory even on error paths

---

## Task: Write to a File

Write data from memory buffer to a file.

```c
#include "file/file.h"
#include "memory/memory.h"
#include "console/console.h"

int write_file_example(void)
{
    // STEP 1: Prepare data to write
    MemoryResult mem_result = fun_memory_allocate(256);
    if (fun_error_is_error(mem_result.error)) {
        return 1;
    }
    Memory buffer = mem_result.value;
    
    // Copy or fill buffer with data
    const char *data = "Hello, World!";
    fun_string_copy(data, buffer);
    
    // STEP 2: Write to file
    AsyncResult write_result = fun_write_memory_to_file((Write){
        .file_path = "output.txt",
        .input = buffer,
        .bytes_to_write = 13,  // Length of data
        .offset = 0
    });
    
    // STEP 3: Wait and check
    fun_async_await(&write_result);
    
    if (write_result.status == ASYNC_ERROR) {
        fun_console_error_line("Failed to write file");
        fun_memory_free(&buffer);
        return 1;
    }
    
    fun_console_write_line("File written successfully");
    
    // STEP 4: Cleanup
    fun_memory_free(&buffer);
    
    return 0;
}
```

**Key Points:**
- Specify exact `bytes_to_write` - won't null-terminate automatically
- File is overwritten if it exists (use append for adding to existing files)
- Parent directories must exist (create them first if needed)

---

## Task: Check if File Exists

Check whether a file exists at the specified path.

```c
#include "filesystem/filesystem.h"
#include "console/console.h"

int check_file_exists_example(void)
{
    // Check if file exists
    BoolResult exists_result = fun_file_exists("path/to/file.txt");
    
    if (fun_error_is_error(exists_result.error)) {
        fun_console_error_line("Error checking file existence");
        return 1;
    }
    
    if (exists_result.value) {
        fun_console_write_line("File exists");
    } else {
        fun_console_write_line("File does not exist");
    }
    
    return 0;
}
```

**Note:** `fun_file_exists()` is part of the path-type-refactor change. Until then, check for file existence by attempting to open and handling the error.

**Alternative (current API):**
```c
// Try to read file - if it fails with NOT_FOUND, it doesn't exist
AsyncResult result = fun_read_file_in_memory((Read){
    .file_path = "path/to/file.txt",
    .output = buffer,
    .bytes_to_read = 1
});
fun_async_await(&result);

if (result.status == ASYNC_ERROR) {
    // File doesn't exist or can't be read
}
```

---

## Task: Append to a File

Append data to the end of an existing file.

```c
#include "file/file.h"
#include "memory/memory.h"

int append_file_example(void)
{
    // Prepare data to append
    MemoryResult mem_result = fun_memory_allocate(128);
    if (fun_error_is_error(mem_result.error)) {
        return 1;
    }
    Memory buffer = mem_result.value;
    
    const char *data = "\nAppended line";
    fun_string_copy(data, buffer);
    
    // Append to file (adds to end)
    AsyncResult append_result = fun_append_memory_to_file((Append){
        .file_path = "log.txt",
        .input = buffer,
        .bytes_to_append = 14  // Length of data
    });
    
    fun_async_await(&append_result);
    
    if (append_result.status == ASYNC_ERROR) {
        fun_console_error_line("Failed to append");
        fun_memory_free(&buffer);
        return 1;
    }
    
    fun_memory_free(&buffer);
    return 0;
}
```

**Key Points:**
- Creates file if it doesn't exist
- Data is added to end of file (unlike write which overwrites)
- Useful for logging, accumulating data

---

## Task: Stream-Based File Reading (Large Files)

For files larger than available memory, use streaming.

```c
#include "stream/stream.h"
#include "memory/memory.h"
#include "console/console.h"

int stream_read_example(void)
{
    // STEP 1: Allocate buffer for stream
    MemoryResult mem_result = fun_memory_allocate(4096);
    if (fun_error_is_error(mem_result.error)) {
        return 1;
    }
    
    // STEP 2: Create file stream
    AsyncResult open_result = fun_stream_create_file_read(
        "large_file.txt",
        mem_result.value,
        4096,  // Buffer size
        FILE_MODE_STANDARD
    );
    fun_async_await(&open_result);
    
    if (open_result.status == ASYNC_ERROR) {
        fun_console_error_line("Failed to open file");
        fun_memory_free(&mem_result.value);
        return 1;
    }
    
    FileStream *stream = (FileStream *)open_result.state;
    
    // STEP 3: Read in chunks
    while (fun_stream_can_read(stream)) {
        uint64_t bytes_read;
        AsyncResult read_result = fun_stream_read(stream, &bytes_read);
        fun_async_await(&read_result);
        
        if (read_result.status == ASYNC_ERROR) {
            break;
        }
        
        // Process chunk (buffer contains bytes_read bytes)
        // ... process data ...
    }
    
    // STEP 4: Cleanup
    fun_stream_destroy(stream);
    fun_memory_free(&mem_result.value);
    
    return 0;
}
```

**Key Points:**
- Use for files too large to fit in memory
- Process data in chunks (4096 bytes in this example)
- Must destroy stream AND free buffer

---

## Error Handling

Common file operation error codes:

| Error Code | Meaning | Action |
|------------|---------|--------|
| `ERROR_CODE_NO_ERROR` | Success | Continue normally |
| `ERROR_CODE_DIRECTORY_NOT_FOUND` | Path doesn't exist | Create directory or check path |
| `ERROR_CODE_PERMISSION_DENIED` | No read/write access | Check permissions |
| `ERROR_CODE_BUFFER_TOO_SMALL` | Buffer insufficient | Allocate larger buffer |
| `ERROR_CODE_NULL_POINTER` | NULL parameter | Check all parameters |

**Pattern for all file operations:**

```c
AsyncResult result = fun_read_file_in_memory(params);
fun_async_await(&result);

if (result.status == ASYNC_ERROR) {
    // Handle error based on result.error.code
    switch (result.error.code) {
        case ERROR_CODE_DIRECTORY_NOT_FOUND:
            // File path doesn't exist
            break;
        case ERROR_CODE_PERMISSION_DENIED:
            // No access
            break;
        default:
            // Other error
            break;
    }
    return 1;
}

// Success - use the data
```

---

## Memory Management

**Always follow this pattern:**

```c
// 1. Allocate
MemoryResult r = fun_memory_allocate(size);
if (fun_error_is_error(r.error)) return 1;

// 2. Use with file operations
// ... file read/write ...

// 3. Free (even on error paths!)
fun_memory_free(&r.value);
```

**Common Pitfalls:**

❌ **Forgetting to free on error:**
```c
MemoryResult r = fun_memory_allocate(1024);
AsyncResult read = fun_read_file_in_memory(...);
fun_async_await(&read);
if (read.status == ASYNC_ERROR) {
    return 1;  // LEAK! Buffer not freed
}
fun_memory_free(&r.value);
```

✅ **Correct - free on all paths:**
```c
MemoryResult r = fun_memory_allocate(1024);
AsyncResult read = fun_read_file_in_memory(...);
fun_async_await(&read);
if (read.status == ASYNC_ERROR) {
    fun_memory_free(&r.value);  // Free before return
    return 1;
}
fun_memory_free(&r.value);
```

---

## Buffer Sizing Guidelines

| Use Case | Recommended Size |
|----------|-----------------|
| Small config files | 256-512 bytes |
| Text files | 4096 bytes (4KB) |
| Binary files | 8192-16384 bytes |
| Stream buffer | 4096 bytes (reused) |
| Unknown size | Start with 4096, check if buffer too small |

---

## See Also

- **[fundamental-memory.md](fundamental-memory.md)** - Memory allocation patterns
- **[fundamental-directory.md](fundamental-directory.md)** - Directory operations
- **[fundamental-stream.md](fundamental-stream.md)** - Stream-based I/O
- **[file/file.h](../../include/file/file.h)** - Complete API reference
