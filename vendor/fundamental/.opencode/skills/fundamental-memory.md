---
name: fundamental-memory
description: Memory management with Fundamental Library - allocate, free, copy, fill, compare
license: MIT
compatibility: Complements fundamental-expert skill
metadata:
  author: fundamental-library
  version: "1.0"
  category: memory-management
  related: fundamental-file-io, fundamental-collections
---

# Fundamental Library - Memory Management Skill

I provide copy-paste examples for memory operations using the Fundamental Library.

---

## Quick Reference

| Task | Function | Example |
|------|----------|---------|
| Allocate | `fun_memory_allocate()` | See below |
| Free | `fun_memory_free()` | See below |
| Copy | `fun_memory_copy()` | See below |
| Fill | `fun_memory_fill()` | See below |
| Compare | `fun_memory_compare()` | See below |

**See Also:** [fundamental-file-io](fundamental-file-io.md) for file buffers, [fundamental-collections](fundamental-collections.md) for data structures

---

## Task: Allocate Memory

Allocate memory for use with Fundamental Library functions.

```c
#include "memory/memory.h"
#include "console/console.h"

int allocate_example(void)
{
    // STEP 1: Allocate memory
    MemoryResult result = fun_memory_allocate(1024);  // 1KB
    
    // STEP 2: Check for allocation failure
    if (fun_error_is_error(result.error)) {
        fun_console_error_line("Memory allocation failed");
        return 1;
    }
    
    // STEP 3: Use the memory
    Memory buffer = result.value;
    // ... use buffer ...
    
    // STEP 4: Free when done
    voidResult free_result = fun_memory_free(&buffer);
    
    // STEP 5: Verify free succeeded (optional, rarely fails)
    if (fun_error_is_error(free_result.error)) {
        fun_console_error_line("Memory free failed");
        return 1;
    }
    
    return 0;
}
```

**Key Points:**
- Always check `fun_error_is_error(result.error)` after allocation
- `Memory` is a typedef for `void *` - treat as opaque pointer
- Free memory even if you encounter errors during use
- After successful free, `buffer` is set to NULL

---

## Task: Free Memory

Deallocate previously allocated memory.

```c
#include "memory/memory.h"

void free_example(void)
{
    // Allocate first
    MemoryResult r = fun_memory_allocate(256);
    if (fun_error_is_error(r.error)) {
        return;
    }
    Memory ptr = r.value;
    
    // ... use ptr ...
    
    // Free the memory
    voidResult result = fun_memory_free(&ptr);
    
    // ptr is now NULL after successful free
    // Check error only if needed (free rarely fails)
    if (fun_error_is_error(result.error)) {
        // Handle error
    }
}
```

**Pointer Handling:**
- Pass **address** of Memory variable: `fun_memory_free(&ptr)`
- After free, `ptr` becomes `NULL` automatically
- Never use memory after freeing (use-after-free bug)
- Never free the same pointer twice (double-free bug)

---

## Task: Copy Memory

Copy bytes from source to destination.

```c
#include "memory/memory.h"

void copy_example(void)
{
    // Allocate source and destination
    MemoryResult src_result = fun_memory_allocate(128);
    MemoryResult dst_result = fun_memory_allocate(128);
    
    if (fun_error_is_error(src_result.error) || 
        fun_error_is_error(dst_result.error)) {
        fun_memory_free(&src_result.value);
        fun_memory_free(&dst_result.value);
        return;
    }
    
    // Fill source with data first
    fun_memory_fill(src_result.value, 0x42, 128);
    
    // Copy 128 bytes from source to destination
    fun_memory_copy(dst_result.value, src_result.value, 128);
    
    // Cleanup
    fun_memory_free(&src_result.value);
    fun_memory_free(&dst_result.value);
}
```

**Key Points:**
- Destination must be pre-allocated with sufficient size
- No overlap checking - for overlapping regions, use careful ordering
- Size is in bytes
- Equivalent to `memcpy()` but without stdlib

**Function Signature:**
```c
void fun_memory_copy(void *destination, const void *source, size_t size);
```

---

## Task: Fill Memory

Set all bytes in a memory region to a specific value.

```c
#include "memory/memory.h"

void fill_example(void)
{
    MemoryResult result = fun_memory_allocate(256);
    if (fun_error_is_error(result.error)) {
        return;
    }
    Memory buffer = result.value;
    
    // Zero out the buffer (common pattern)
    fun_memory_fill(buffer, 0, 256);
    
    // Or fill with a specific byte value
    fun_memory_fill(buffer, 0xFF, 256);  // All bits set
    
    fun_memory_free(&buffer);
}
```

**Common Patterns:**

```c
// Zero initialization (most common)
fun_memory_fill(buffer, 0, size);

// Set all bits to 1
fun_memory_fill(buffer, 0xFF, size);

// Initialize with a specific byte pattern
fun_memory_fill(buffer, 0x42, size);  // 0x42 = 66 = 'B'
```

**Key Points:**
- Fills `size` bytes with the specified value
- Each byte gets the same value (value is truncated to uint8_t)
- Equivalent to `memset()` but without stdlib

---

## Task: Compare Memory

Compare two memory regions for equality.

```c
#include "memory/memory.h"
#include "console/console.h"

void compare_example(void)
{
    MemoryResult a_result = fun_memory_allocate(64);
    MemoryResult b_result = fun_memory_allocate(64);
    
    // Fill both with same data
    fun_memory_fill(a_result.value, 0x42, 64);
    fun_memory_fill(b_result.value, 0x42, 64);
    
    // Compare
    int32_t cmp = fun_memory_compare(a_result.value, b_result.value, 64);
    
    if (cmp == 0) {
        fun_console_write_line("Buffers are equal");
    } else if (cmp < 0) {
        fun_console_write_line("Buffer A is less than Buffer B");
    } else {
        fun_console_write_line("Buffer A is greater than Buffer B");
    }
    
    fun_memory_free(&a_result.value);
    fun_memory_free(&b_result.value);
}
```

**Return Value:**
- `0` - Buffers are equal
- `< 0` - First differing byte in A is less than B
- `> 0` - First differing byte in A is greater than B

**Key Points:**
- Compares `size` bytes
- Returns on first differing byte
- Equivalent to `memcmp()` but without stdlib

---

## Memory Safety

### Prevent Use-After-Free

❌ **Wrong:**
```c
MemoryResult r = fun_memory_allocate(128);
fun_memory_free(&r.value);
// BUG: Using freed memory!
fun_memory_fill(r.value, 0, 128);
```

✅ **Correct:**
```c
MemoryResult r = fun_memory_allocate(128);
fun_memory_fill(r.value, 0, 128);  // Use BEFORE free
fun_memory_free(&r.value);
// r.value is now NULL - safe to check
if (r.value == NULL) {
    // Expected - memory was freed
}
```

### Prevent Double-Free

❌ **Wrong:**
```c
MemoryResult r = fun_memory_allocate(128);
fun_memory_free(&r.value);
// BUG: Freeing again!
fun_memory_free(&r.value);
```

✅ **Correct:**
```c
MemoryResult r = fun_memory_allocate(128);
fun_memory_free(&r.value);
r.value = NULL;  // Explicit nullify (already done by free, but explicit is safe)
// Safe - won't double-free NULL
```

### Prevent Memory Leaks

❌ **Wrong - leak on error path:**
```c
MemoryResult r = fun_memory_allocate(1024);
if (some_check_fails()) {
    return 1;  // LEAK! Memory not freed
}
fun_memory_free(&r.value);
```

✅ **Correct - free on all paths:**
```c
MemoryResult r = fun_memory_allocate(1024);
if (some_check_fails()) {
    fun_memory_free(&r.value);  // Free before return
    return 1;
}
fun_memory_free(&r.value);
```

---

## Buffer Sizing Guidelines

| Use Case | Size | Notes |
|----------|------|-------|
| Small string | 64-256 bytes | Short text, paths |
| Line buffer | 512-1024 bytes | Console input/output |
| File read | 4096 bytes | Common page size |
| File stream | 4096-8192 bytes | Chunked I/O |
| Large data | 16384+ bytes | Binary blobs |

**Calculate size for strings:**
```c
// For a string, allocate length + 1 for null terminator
size_t len = fun_string_length(my_string);
MemoryResult r = fun_memory_allocate(len + 1);
```

---

## See Also

- **[fundamental-file-io.md](fundamental-file-io.md)** - File buffers and I/O
- **[fundamental-string.md](fundamental-string.md)** - String operations
- **[fundamental-collections.md](fundamental-collections.md)** - Data structures
- **[memory/memory.h](../../include/memory/memory.h)** - Complete API reference
