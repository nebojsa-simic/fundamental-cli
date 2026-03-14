---
name: fundamental-string
description: String operations with Fundamental Library - copy, join, template, convert
license: MIT
compatibility: Complements fundamental-expert skill
metadata:
  author: fundamental-library
  version: "1.0"
  category: string-operations
  related: fundamental-memory, fundamental-console
---

# Fundamental Library - String Operations Skill

I provide copy-paste examples for string operations using the Fundamental Library.

---

## Quick Reference

| Task | Function | Example |
|------|----------|---------|
| Copy | `fun_string_copy()` | See below |
| Join | `fun_string_join()` | See below |
| Template | `fun_string_template()` | See below |
| Int to string | `fun_string_from_int()` | See below |
| Double to string | `fun_string_from_double()` | See below |
| Compare | `fun_string_compare()` | See below |
| Length | `fun_string_length()` | See below |
| Trim | `fun_string_trim_in_place()` | See below |

**See Also:** [fundamental-memory](fundamental-memory.md) for buffer allocation

---

## Task: Copy a String

Copy a string to a pre-allocated buffer.

```c
#include "string/string.h"
#include "memory/memory.h"

void string_copy_example(void)
{
    // Allocate destination buffer
    MemoryResult mem_result = fun_memory_allocate(256);
    if (fun_error_is_error(mem_result.error)) {
        return;
    }
    char *dest = (char *)mem_result.value;
    
    // Source string
    const char *source = "Hello, World!";
    
    // Copy (dest must be large enough)
    fun_string_copy(source, dest);
    
    // Use dest...
    
    // Cleanup
    fun_memory_free(&mem_result.value);
}
```

**Key Points:**
- Destination buffer must be pre-allocated
- Buffer must be large enough for source + null terminator
- No bounds checking - ensure buffer is big enough

---

## Task: Join Two Strings

Concatenate two strings.

```c
#include "string/string.h"
#include "memory/memory.h"

void string_join_example(void)
{
    // Allocate buffer for result
    MemoryResult mem_result = fun_memory_allocate(512);
    if (fun_error_is_error(mem_result.error)) {
        return;
    }
    char *result = (char *)mem_result.value;
    
    // Join two strings
    fun_string_join("Hello, ", "World!", result);
    
    // result = "Hello, World!"
    
    fun_memory_free(&mem_result.value);
}
```

**Key Points:**
- Result buffer must hold both strings + null terminator
- No separator added between strings (add explicitly if needed)

---

## Task: String Templates

Format strings with placeholders.

```c
#include "string/string.h"
#include "memory/memory.h"

void string_template_example(void)
{
    // Allocate output buffer
    MemoryResult mem_result = fun_memory_allocate(512);
    if (fun_error_is_error(mem_result.error)) {
        return;
    }
    char *output = (char *)mem_result.value;
    
    // Template with placeholders: ${string} #{int} %{double}
    const char *template_str = "User ${name} is #{age} years old, score: %{score}";
    
    // Prepare parameters
    StringTemplateParam params[] = {
        { "name", { .stringValue = "Alice" } },
        { "age", { .intValue = 30 } },
        { "score", { .doubleValue = 95.5 } }
    };
    
    // Render template
    fun_string_template(template_str, params, 3, output);
    
    // output = "User Alice is 30 years old, score: 95.500000"
    
    fun_memory_free(&mem_result.value);
}
```

**Placeholder Types:**
- `${name}` - String value
- `#{name}` - Integer value
- `%{name}` - Double value
- `*{name}` - Pointer value

**Key Points:**
- Parameter names must match placeholder names
- Output buffer must be large enough
- Double values print with full precision

---

## Task: Convert Integer to String

Format an integer as a string.

```c
#include "string/string.h"
#include "memory/memory.h"

void int_to_string_example(void)
{
    // Allocate buffer
    MemoryResult mem_result = fun_memory_allocate(64);
    if (fun_error_is_error(mem_result.error)) {
        return;
    }
    char *buffer = (char *)mem_result.value;
    
    // Convert int to string (base 10)
    int64_t num = -42;
    fun_string_from_int(num, 10, buffer);
    // buffer = "-42"
    
    // Convert to hex (base 16)
    fun_string_from_int(255, 16, buffer);
    // buffer = "ff"
    
    fun_memory_free(&mem_result.value);
}
```

**Supported Bases:**
- 10 - Decimal
- 16 - Hexadecimal (lowercase)
- 8 - Octal
- 2 - Binary

---

## Task: Convert Double to String

Format a double-precision float as a string.

```c
#include "string/string.h"
#include "memory/memory.h"

void double_to_string_example(void)
{
    MemoryResult mem_result = fun_memory_allocate(64);
    if (fun_error_is_error(mem_result.error)) {
        return;
    }
    char *buffer = (char *)mem_result.value;
    
    // Convert double to string (6 decimal places)
    double num = 3.14159;
    fun_string_from_double(num, 6, buffer);
    // buffer = "3.141590"
    
    fun_memory_free(&mem_result.value);
}
```

**Key Points:**
- Second parameter is decimal places after point
- Default precision varies by implementation

---

## Task: Compare Strings

Compare two strings for equality or ordering.

```c
#include "string/string.h"
#include "console/console.h"

void string_compare_example(void)
{
    const char *a = "apple";
    const char *b = "banana";
    const char *c = "apple";
    
    int64_t cmp = fun_string_compare(a, b);
    
    if (cmp == 0) {
        fun_console_write_line("Strings are equal");
    } else if (cmp < 0) {
        fun_console_write_line("a comes before b");
    } else {
        fun_console_write_line("a comes after b");
    }
    
    // Check equality
    if (fun_string_compare(a, c) == 0) {
        fun_console_write_line("a and c are equal");
    }
}
```

**Return Values:**
- `0` - Strings are equal
- `< 0` - First string comes before second (lexicographically)
- `> 0` - First string comes after second

---

## Task: Get String Length

Get the length of a string.

```c
#include "string/string.h"

void string_length_example(void)
{
    const char *str = "Hello, World!";
    
    StringLength len = fun_string_length(str);
    // len = 13 (not including null terminator)
    
    // Use len for buffer sizing
    MemoryResult r = fun_memory_allocate(len + 1);  // +1 for null
}
```

**Key Points:**
- Returns count of characters (not including null terminator)
- Equivalent to `strlen()` but without stdlib

---

## Task: Trim Whitespace

Remove leading and trailing whitespace in-place.

```c
#include "string/string.h"
#include "memory/memory.h"

void string_trim_example(void)
{
    // Allocate and copy string (must be mutable for trim)
    MemoryResult mem_result = fun_memory_allocate(128);
    if (fun_error_is_error(mem_result.error)) {
        return;
    }
    char *buffer = (char *)mem_result.value;
    
    // Copy string to buffer
    fun_string_copy("  Hello, World!  ", buffer);
    
    // Trim in place
    fun_string_trim_in_place(buffer);
    // buffer = "Hello, World!"
    
    fun_memory_free(&mem_result.value);
}
```

**Key Points:**
- Modifies string in-place
- Removes spaces, tabs, newlines from both ends
- String must be mutable (not a string literal)

---

## Common Patterns

### Build Dynamic Message
```c
MemoryResult r = fun_memory_allocate(512);
char *msg = (char *)r.value;

fun_string_copy("Processing: ", msg);
// ... append more data ...

fun_console_write_line(msg);
fun_memory_free(&r.value);
```

### Format Number with Text
```c
MemoryResult r = fun_memory_allocate(128);
char *buffer = (char *)r.value;

fun_string_from_int(count, 10, buffer);
fun_string_join("Count: ", buffer, buffer);
fun_string_join(buffer, " items", buffer);

fun_console_write_line(buffer);
fun_memory_free(&r.value);
```

### Template for CLI Output
```c
MemoryResult r = fun_memory_allocate(256);
char *output = (char *)r.value;

const char *tpl = "Name: ${name}, ID: #{id}";
StringTemplateParam params[] = {
    { "name", { .stringValue = user_name } },
    { "id", { .intValue = user_id } }
};

fun_string_template(tpl, params, 2, output);
fun_console_write_line(output);

fun_memory_free(&r.value);
```

---

## Buffer Sizing

| String Type | Recommended Size |
|-------------|-----------------|
| Short text | 64-128 bytes |
| Line of text | 256-512 bytes |
| Template output | 512-1024 bytes |
| Integer (any base) | 64 bytes (plenty for 64-bit) |
| Double | 64 bytes |

**Calculate exact size:**
```c
size_t needed = fun_string_length(str1) + fun_string_length(str2) + 1;
MemoryResult r = fun_memory_allocate(needed);
```

---

## See Also

- **[fundamental-memory.md](fundamental-memory.md)** - Buffer allocation
- **[fundamental-console.md](fundamental-console.md)** - Output formatting
- **[string/string.h](../../include/string/string.h)** - Complete API reference
