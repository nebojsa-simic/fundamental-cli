---
name: fundamental-console
description: Console output with Fundamental Library - write text, progress bars, error messages
license: MIT
compatibility: Complements fundamental-expert skill
metadata:
  author: fundamental-library
  version: "1.0"
  category: console-output
  related: fundamental-string
---

# Fundamental Library - Console Output Skill

I provide copy-paste examples for console operations using the Fundamental Library.

---

## Quick Reference

| Task | Function | Example |
|------|----------|---------|
| Write text | `fun_console_write()` | See below |
| Write line | `fun_console_write_line()` | See below |
| Error output | `fun_console_error_line()` | See below |
| Flush buffer | `fun_console_flush()` | See below |
| Progress bar | Custom implementation | See below |

**See Also:** [fundamental-string](fundamental-string.md) for formatting text

---

## Task: Write Text to Console

Output text without a trailing newline.

```c
#include "console/console.h"

void write_example(void)
{
    // Write without newline
    fun_console_write("Hello");
    fun_console_write(" ");
    fun_console_write("World");
    
    // Don't forget to flush for immediate output
    fun_console_flush();
}
```

**Key Points:**
- No newline added - cursor stays at end of text
- Output is buffered - call `fun_console_flush()` for immediate display
- Use for inline updates, progress indicators

---

## Task: Write Line to Console

Output text with a trailing newline.

```c
#include "console/console.h"

void write_line_example(void)
{
    // Write with newline
    fun_console_write_line("Hello, World!");
    fun_console_write_line("Second line");
    
    // Flush to ensure output appears
    fun_console_flush();
}
```

**Key Points:**
- Automatically adds newline at end
- Most common output function
- Still buffered - flush for immediate output

---

## Task: Write Error Messages

Output error messages to stderr.

```c
#include "console/console.h"

void error_example(void)
{
    // Write error message to stderr
    fun_console_error_line("An error occurred!");
    
    // Error output is typically unbuffered (appears immediately)
}
```

**Key Points:**
- Outputs to stderr (file descriptor 2)
- Automatically adds newline
- Use for error messages, warnings
- Typically appears even if stdout is redirected

---

## Task: Flush Console Buffer

Force buffered output to appear immediately.

```c
#include "console/console.h"

void flush_example(void)
{
    fun_console_write("Processing");
    fun_console_flush();  // Force output now
    
    // ... do work ...
    
    fun_console_write_line(" done!");
    fun_console_flush();
}
```

**When to Flush:**
- Before long operations (so user sees progress)
- In progress bar updates
- Before waiting for user input
- Before potential crash points (for debugging)

---

## Task: Implement Progress Bar

Display a progress bar for long-running operations.

```c
#include "console/console.h"
#include "memory/memory.h"
#include "string/string.h"

void progress_bar_example(int total_items)
{
    // Allocate buffer for progress bar display
    MemoryResult mem_result = fun_memory_allocate(256);
    if (fun_error_is_error(mem_result.error)) {
        return;
    }
    char *buffer = (char *)mem_result.value;
    
    int completed = 0;
    
    for (int i = 0; i < total_items; i++) {
        // ... do work ...
        completed++;
        
        // Calculate percentage
        int percent = (completed * 100) / total_items;
        
        // Build progress bar string
        int bar_width = 20;
        int filled = (completed * bar_width) / total_items;
        
        // Create bar: [=====>    ]
        int pos = 0;
        buffer[pos++] = '[';
        
        for (int j = 0; j < bar_width; j++) {
            if (j < filled) {
                buffer[pos++] = '=';
            } else if (j == filled) {
                buffer[pos++] = '>';
            } else {
                buffer[pos++] = ' ';
            }
        }
        
        buffer[pos++] = ']';
        buffer[pos++] = ' ';
        
        // Add percentage
        fun_string_from_int(percent, 10, buffer + pos);
        pos += fun_string_length(buffer + pos);
        buffer[pos++] = '%';
        buffer[pos++] = '\0';
        
        // Output with carriage return to overwrite
        fun_console_write("\r");
        fun_console_write(buffer);
        fun_console_flush();
    }
    
    // Final newline after progress completes
    fun_console_write_line("");
    
    // Cleanup
    fun_memory_free(&mem_result.value);
}
```

**Usage:**
```c
// Process 100 items with progress bar
progress_bar_example(100);
```

**Output:**
```
[====================] 100%
```

**Key Points:**
- Use `\r` (carriage return) to return to start of line
- Overwrite previous progress display
- Always add final newline when complete
- Flush after each update for smooth animation

---

## Task: Simple Progress Counter

Alternative to progress bar - simple counter.

```c
#include "console/console.h"
#include "string/string.h"
#include "memory/memory.h"

void progress_counter_example(int total)
{
    MemoryResult mem_result = fun_memory_allocate(64);
    if (fun_error_is_error(mem_result.error)) {
        return;
    }
    char *buffer = (char *)mem_result.value;
    
    for (int i = 0; i < total; i++) {
        // ... do work ...
        
        // Update counter: "Processed 42/100"
        fun_string_from_int(i + 1, 10, buffer);
        char *num_str = buffer;
        
        fun_console_write("\rProcessed ");
        fun_console_write(num_str);
        fun_console_write("/");
        fun_string_from_int(total, 10, buffer);
        fun_console_write(buffer);
        fun_console_flush();
    }
    
    fun_console_write_line("");
    fun_memory_free(&mem_result.value);
}
```

---

## Console Output Buffering

**Understanding buffering:**

```c
// Output is BUFFERED - may not appear immediately
fun_console_write_line("Starting...");

// Long operation - user sees nothing yet!
do_long_operation();

// Flush - now user sees "Starting..."
fun_console_flush();

fun_console_write_line("Done!");
fun_console_flush();
```

**Best practice - flush at logical points:**
```c
fun_console_write_line("Step 1: Initializing...");
fun_console_flush();
initialize();

fun_console_write_line("Step 2: Processing...");
fun_console_flush();
process();

fun_console_write_line("Complete!");
fun_console_flush();
```

---

## Common Patterns

### Status Messages
```c
fun_console_write_line("[INFO] Loading configuration...");
fun_console_flush();
```

### Warnings
```c
fun_console_write_line("[WARN] Config file not found, using defaults");
fun_console_flush();
```

### Errors
```c
fun_console_error_line("[ERROR] Failed to connect to database");
```

### Success Messages
```c
fun_console_write_line("[OK] Operation completed successfully");
fun_console_flush();
```

---

## See Also

- **[fundamental-string.md](fundamental-string.md)** - String formatting for output
- **[fundamental-file-io.md](fundamental-file-io.md)** - File I/O examples
- **[console/console.h](../../include/console/console.h)** - Complete API reference
