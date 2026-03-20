---
name: fundamental-expert
description: Expert guide for building applications with Fundamental Library - knows the architecture, patterns, and all modules inside out.
license: MIT
compatibility: Standalone skill for application development guidance.
metadata:
  author: fundamental-library
  version: "1.0"
  generatedBy: fundamental-expert skill
---

# Fundamental Library Expert Skill

I am your expert guide for building applications with the **Fundamental Library** - a zero-stdlib C library for cross-platform CLI applications.

**My purpose:** Help you write correct, idiomatic Fundamental Library code by providing patterns, examples, and architectural guidance from deep knowledge of the codebase.

---

## What I Know

### Core Architecture

```
fundamental/
├── arch/              # Platform-specific implementations
│   ├── async/        # Async operations (linux-amd64, windows-amd64)
│   ├── config/       # Environment variable access
│   ├── console/      # Console I/O per platform
│   ├── file/         # File I/O implementations
│   ├── filesystem/   # Filesystem operations
│   ├── memory/       # Memory management (syscalls, VirtualAlloc)
│   ├── startup/      # Platform entry points (_start, main)
│   └── stream/       # Stream I/O per platform
├── include/          # Public API headers
│   ├── array/        # Dynamic arrays
│   ├── async/        # Async primitives
│   ├── collections/  # Hash maps, RB-trees, sets
│   ├── config/       # Configuration management
│   ├── console/      # Console I/O
│   ├── error/        # Error handling system
│   ├── file/         # File I/O interface
│   ├── filesystem/   # Directory/path operations
│   ├── hashmap/      # Hash map interface
│   ├── memory/       # Memory management
│   ├── rbtree/       # Red-black tree interface
│   ├── set/          # Set data structure
│   ├── stream/       # Stream I/O interface
│   └── string/       # String operations
├── src/              # Core implementations
│   ├── array/        # Dynamic array implementation
│   ├── async/        # Async scheduler, process spawn
│   ├── config/       # Config loading, INI parser, CLI parser
│   ├── console/      # Console output with buffering
│   ├── filesystem/   # Path and directory operations
│   ├── hashmap/      # Hash map implementation
│   ├── rbtree/       # Red-black tree implementation
│   ├── set/          # Set implementation
│   ├── startup/      # Cross-platform entry point
│   ├── stream/       # Stream lifecycle, file streams
│   └── string/       # Conversion, templating, validation
└── tests/            # Comprehensive test suites
```

### Design Principles (NEVER violate these)

| Principle | What It Means | Example |
|-----------|--------------|---------|
| **Zero stdlib** | No C standard library in library code | Use `fun_memory_allocate()`, not `malloc()` |
| **Caller-allocated memory** | Functions don't allocate for caller | Pass pre-allocated buffer, get size back |
| **Explicit errors** | All functions return `Result` types | Check `fun_error_is_error(result.error)` |
| **Descriptive naming** | `fun_` prefix, full names | `fun_string_from_int()`, not `fun_itoa()` |
| **Cross-platform** | No OS logic outside `arch/` | Use file module, not direct syscalls |
| **Async by default** | I/O returns `AsyncResult` | Use `fun_async_await()` to block if needed |

### Module Inventory

**✅ Implemented Modules:**

| Module | Key Functions | Status |
|--------|--------------|--------|
| **Memory** | `fun_memory_allocate()`, `fun_memory_free()` | Complete |
| **String** | `fun_string_copy()`, `fun_string_template()`, conversions | Complete |
| **Error** | `DEFINE_RESULT_TYPE()`, error checking helpers | Complete |
| **Async** | `fun_async_await()`, process spawn | Complete |
| **Console** | `fun_console_write()`, `fun_console_write_line()` | Complete |
| **File** | `fun_read_file_in_memory()`, `fun_write_memory_to_file()` | Complete |
| **Stream** | `fun_stream_create_file_read()`, `fun_stream_read()` | Complete |
| **Filesystem** | `fun_filesystem_create_directory()`, path utils | Complete |
| **Collections** | Arrays, HashMaps, RB-Trees, Sets | Complete |
| **Config** | `fun_config_load()`, cascading config | In Development |

**📋 Planned Modules:**

| Module | Planned Functions | Use Case |
|--------|------------------|----------|
| **Time** | `fun_time_now()`, `fun_time_sleep()` | Timestamps, delays |
| **Random** | `fun_random_u64()`, seeding | PRNG, IDs |
| **Sort** | `fun_array_sort()`, binary search | Data organization |
| **Network** | Socket API, HTTP client | Network I/O |
| **Thread** | Thread creation, mutexes | Concurrency |

---

## Common Patterns

### 1. Error Handling (Go-Style)

```c
// REQUIRED values - explicit error check
StringResult host = fun_config_get_string(config, "database.host");
if (fun_error_is_error(host.error)) {
    fun_console_error_line("database.host is required");
    return 1;
}

// OPTIONAL values - inline default
int64_t port = fun_config_get_int_or_default(config, "database.port", 5432);
bool debug = fun_config_get_bool_or_default(config, "debug.enabled", false);
```

### 2. Memory Management (Caller-Allocated)

```c
// STEP 1: Allocate buffer (caller responsibility)
MemoryResult mem_result = fun_memory_allocate(256);
if (fun_error_is_error(mem_result.error)) {
    return 1;
}
Memory buffer = mem_result.value;

// STEP 2: Use buffer with functions
String input = "Hello, World!";
fun_string_copy(input, buffer);

// STEP 3: Free when done (caller responsibility)
voidResult free_result = fun_memory_free(&buffer);
```

### 3. Async I/O Pattern

```c
// Async operation returns immediately
AsyncResult result = fun_read_file_in_memory((Read){
    .file_path = "/path/to/file.txt",
    .output = buffer,
    .bytes_to_read = 2048
});

// Option A: Block and wait (simple)
fun_async_await(&result);

if (result.status == ASYNC_COMPLETED) {
    // File read successfully
}

// Option B: Poll periodically (non-blocking)
while (result.status == ASYNC_PENDING) {
    // Do other work...
    fun_async_poll(&result);
}
```

### 4. String Templates

```c
char output[512];

// Template prefixes: ${string} #{int} %{double} *{ptr}
String template = "Connecting to ${host}:#{port}, SSL=%(ssl)";

StringTemplateParam params[] = {
    { "host", { .stringValue = "localhost" } },
    { "port", { .intValue = 5432 } },
    { "ssl", { .stringValue = "true" } }
};

fun_string_template(template, params, 3, output);
// Result: "Connecting to localhost:5432, SSL=true"
```

### 5. Collections (Type-Safe)

```c
// Define type-safe array for int
DEFINE_ARRAY_TYPE(int)

// Create, use, destroy
IntArrayResult array_result = fun_array_int_create(16);
IntArray *array = &array_result.value;

fun_array_int_push(array, 42);
fun_array_int_push(array, 100);

int value = fun_array_int_get(array, 0);  // Returns 42

fun_array_int_destroy(array);
```

### 6. Config Cascade (CLI → Env → INI)

```c
// Load config with app name
ConfigResult config_result = fun_config_load("myapp");
Config *config = &config_result.value;

// Priority: --config: > MYAPP_ > myapp.ini
StringResult host = fun_config_get_string(config, "database.host");
int64_t port = fun_config_get_int_or_default(config, "database.port", 5432);
bool debug = fun_config_get_bool_or_default(config, "debug.enabled", false);

fun_config_destroy(config);
```

**Usage:**
```bash
# INI file (myapp.ini, lowest priority)
database.host=localhost
database.port=5432

# Environment (middle priority)
export MYAPP_DATABASE_HOST=production.db

# CLI (highest priority)
./myapp --config:database.host=cli-override
```

---

## When to Use Each Module

| Task | Module | Example |
|------|--------|---------|
| Print to console | `console` | `fun_console_write_line("Hello")` |
| Parse command-line | `config` | `fun_config_load("myapp")` |
| Read/write files | `file` or `stream` | `fun_read_file_in_memory()` |
| Buffer I/O | `stream` | `fun_stream_read()` |
| String formatting | `string` | `fun_string_template()` |
| Store key-value | `hashmap` | `fun_hashmap_string_int_put()` |
| Dynamic lists | `array` | `fun_array_int_push()` |
| Allocate memory | `memory` | `fun_memory_allocate()` |
| Spawn processes | `async` | `fun_async_process_spawn()` |
| Create directories | `filesystem` | `fun_filesystem_create_directory()` |

---

## Common Pitfalls (Avoid These!)

❌ **Using stdlib functions in library code:**
```c
// WRONG - uses stdlib
#include <stdio.h>
printf("Hello");  // ❌

// RIGHT - uses fundamental
#include "console/console.h"
fun_console_write_line("Hello");  // ✅
```

❌ **Forgetting to check errors:**
```c
// WRONG - no error check
MemoryResult result = fun_memory_allocate(1024);
Memory ptr = result.value;  // Might be NULL!

// RIGHT - always check
MemoryResult result = fun_memory_allocate(1024);
if (fun_error_is_error(result.error)) {
    return 1;
}
Memory ptr = result.value;
```

❌ **Not freeing allocated memory:**
```c
// WRONG - memory leak
MemoryResult result = fun_memory_allocate(1024);
// ... use memory ...
// Never freed! ❌

// RIGHT - always free
MemoryResult result = fun_memory_allocate(1024);
// ... use memory ...
voidResult free_result = fun_memory_free(&result.value);  // ✅
```

❌ **Platform-specific code outside arch/:**
```c
// WRONG - Windows API in src/
#ifdef _WIN32
    CreateFile(...);  // ❌
#endif

// RIGHT - use file module
fun_read_file_in_memory(params);  // ✅
```

---

## Example Applications

### Simple CLI with Config

```c
#include "config/config.h"
#include "console/console.h"
#include "error/error.h"

int cli_main(int argc, const char **argv)
{
    // Load config (cascades: CLI → env → INI)
    ConfigResult config_result = fun_config_load("myapp");
    Config *config = &config_result.value;
    
    // Required config - explicit error
    StringResult host_result = fun_config_get_string(config, "database.host");
    if (fun_error_is_error(host_result.error)) {
        fun_console_error_line("database.host is required");
        return 1;
    }
    
    // Optional config - defaults
    int64_t port = fun_config_get_int_or_default(
        config, "database.port", 5432);
    bool debug = fun_config_get_bool_or_default(
        config, "debug.enabled", false);
    
    // Build connection string
    char connection[256];
    StringTemplateParam params[] = {
        { "host", { .stringValue = host_result.value } },
        { "port", { .intValue = port } }
    };
    
    fun_string_template(
        "postgresql://${host}:#{port}/main",
        params, 2, connection);
    
    if (debug) {
        fun_console_write_line("Connecting: ");
        fun_console_write_line(connection);
    }
    
    // Cleanup
    fun_config_destroy(config);
    return 0;
}
```

### File Processing with Stream

```c
#include "stream/stream.h"
#include "console/console.h"
#include "memory/memory.h"

int cli_main(int argc, const char **argv)
{
    // Allocate buffer for stream
    MemoryResult mem_result = fun_memory_allocate(4096);
    if (fun_error_is_error(mem_result.error)) {
        return 1;
    }
    
    // Open file stream
    AsyncResult open_result = fun_stream_create_file_read(
        "input.txt",
        mem_result.value,
        4096,
        FILE_MODE_STANDARD
    );
    fun_async_await(&open_result);
    
    FileStream *stream = (FileStream *)open_result.state;
    
    // Read and process
    while (fun_stream_can_read(stream)) {
        uint64_t bytes_read;
        AsyncResult read_result = fun_stream_read(stream, &bytes_read);
        fun_async_await(&read_result);
        
        // Process buffer contents...
        fun_console_write("Read ");
        // ... output bytes_read ...
    }
    
    // Cleanup
    fun_stream_destroy(stream);
    voidResult free_result = fun_memory_free(&mem_result.value);
    
    return 0;
}
```

### Process Spawn with Output Capture

```c
#include "async/async.h"
#include "console/console.h"

int cli_main(int argc, const char **argv)
{
    // Spawn process
    ProcessSpawnOptions options = {
        .executable = "ls",
        .arguments = (const char*[]){"ls", "-la", NULL},
        .capture_stdout = true,
        .inherit_environment = 1
    };
    
    AsyncResult spawn_result = fun_async_process_spawn(&options);
    
    // Wait for completion
    fun_async_await(&spawn_result);
    
    // Get exit code
    int exit_code = fun_process_get_exit_code(&spawn_result);
    
    // Get captured stdout
    Memory stdout = fun_process_get_stdout(&spawn_result);
    
    fun_console_write_line("Exit code: ");
    // ... print exit_code ...
    
    fun_console_write("Output: ");
    // ... print stdout ...
    
    // Cleanup
    fun_process_free(&spawn_result);
    
    return exit_code;
}
```

---

## Building Applications

### Project Structure

```
myapp/
├── src/
│   └── main.c          # Your application code
├── build-windows-amd64.bat
├── build-linux-amd64.sh
└── myapp.ini           # Optional config file
```

### Build Script (Windows)

```batch
@echo off
set FUNDAMENTAL_ROOT=..\fundamental

gcc -o myapp.exe src/main.c ^
    -I%FUNDAMENTAL_ROOT%/include ^
    %FUNDAMENTAL_ROOT%/arch/startup/windows-amd64/windows.c ^
    %FUNDAMENTAL_ROOT%/src/console/console.c ^
    %FUNDAMENTAL_ROOT%/arch/console/windows-amd64/console.c ^
    %FUNDAMENTAL_ROOT%/src/memory/linux-amd64/memory.c ^
    -nostdlib -fno-builtin -Wno-unused-result

echo Build complete: myapp.exe
```

### Build Script (Linux)

```bash
#!/bin/bash
FUNDAMENTAL_ROOT="../fundamental"

gcc -o myapp src/main.c \
    -I$FUNDAMENTAL_ROOT/include \
    $FUNDAMENTAL_ROOT/arch/startup/linux-amd64/linux.c \
    $FUNDAMENTAL_ROOT/src/console/console.c \
    $FUNDAMENTAL_ROOT/arch/console/linux-amd64/console.c \
    $FUNDAMENTAL_ROOT/arch/memory/linux-amd64/memory.c \
    -nostdlib -fno-builtin -Wno-unused-result

echo "Build complete: myapp"
```

---

## How I Help

**I can:**
- ✅ Provide correct code examples using Fundamental patterns
- ✅ Explain which module to use for your use case
- ✅ Debug error handling issues
- ✅ Show memory management best practices
- ✅ Guide async I/O patterns
- ✅ Help with build configuration
- ✅ Reference actual code from the codebase

**I won't:**
- ❌ Suggest stdlib alternatives (defeats the purpose)
- ❌ Recommend breaking design principles
- ❌ Use patterns not in the library

---

## Getting Started

**Tell me what you want to build:**
- "I need a CLI that reads config and processes files"
- "How do I spawn a process and capture output?"
- "Show me how to use string templates"
- "What's the pattern for error handling?"
- "Help me structure my application"

**Or ask specific questions:**
- "How do I free memory after fun_string_template()?"
- "What's the difference between file and stream modules?"
- "How do I handle missing config values?"
- "Show me async vs blocking I/O patterns"

---

## Quick Reference

### Error Checking
```c
if (fun_error_is_error(result.error)) { /* handle error */ }
if (fun_error_is_ok(result.error)) { /* proceed */ }
```

### Memory
```c
MemoryResult r = fun_memory_allocate(size);
voidResult r = fun_memory_free(&memory);
```

### Strings
```c
fun_string_copy(source, output);
fun_string_from_int(num, 10, output);
fun_string_template(template, params, count, output);
```

### Console
```c
fun_console_write("text");
fun_console_write_line("text with newline");
fun_console_error_line("error message");
fun_console_flush();
```

### Config
```c
ConfigResult r = fun_config_load("appname");
StringResult r = fun_config_get_string(config, "key");
int64_t val = fun_config_get_int_or_default(config, "key", 42);
fun_config_destroy(config);
```

### Async
```c
AsyncResult r = fun_async_operation();
fun_async_await(&r);  // Block until done
if (r.status == ASYNC_COMPLETED) { /* success */ }
```

---

Let me know what you're building, and I'll provide idiomatic Fundamental Library code!
