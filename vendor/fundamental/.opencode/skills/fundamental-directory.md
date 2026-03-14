---
name: fundamental-directory
description: Directory operations with Fundamental Library - create, list, remove, iterate
license: MIT
compatibility: Complements fundamental-expert skill
metadata:
  author: fundamental-library
  version: "1.0"
  category: directory-operations
  related: fundamental-file-io, fundamental-filesystem
---

# Fundamental Library - Directory Operations Skill

I provide copy-paste examples for directory operations using the Fundamental Library.

---

## Quick Reference

| Task | Function | Example |
|------|----------|---------|
| Create directory | `fun_filesystem_create_directory()` | See below |
| List contents | `fun_filesystem_list_directory()` | See below |
| Remove directory | `fun_filesystem_remove_directory()` | See below |
| Check exists | `fun_directory_exists()` | See below |
| Iterate files | List + parse | See below |

**See Also:** [fundamental-file-io](fundamental-file-io.md) for file operations

---

## Task: Create a Directory

Create a directory, including parent directories.

```c
#include "filesystem/filesystem.h"
#include "console/console.h"

int create_directory_example(void)
{
    // Create directory (creates parents if needed)
    ErrorResult result = fun_filesystem_create_directory("/tmp/test/nested/dir");
    
    if (fun_error_is_error(result)) {
        if (result.code == ERROR_CODE_DIRECTORY_EXISTS) {
            fun_console_write_line("Directory already exists");
        } else if (result.code == ERROR_CODE_PERMISSION_DENIED) {
            fun_console_error_line("Permission denied");
        } else {
            fun_console_error_line("Failed to create directory");
        }
        return 1;
    }
    
    fun_console_write_line("Directory created successfully");
    return 0;
}
```

**Key Points:**
- Creates parent directories automatically (like `mkdir -p`)
- Returns `ERROR_CODE_DIRECTORY_EXISTS` if directory already exists
- Fails if parent path is a file, not a directory

---

## Task: List Directory Contents

List all files and directories in a directory.

```c
#include "filesystem/filesystem.h"
#include "memory/memory.h"
#include "console/console.h"

int list_directory_example(void)
{
    // STEP 1: Allocate buffer for listing
    MemoryResult mem_result = fun_memory_allocate(4096);
    if (fun_error_is_error(mem_result.error)) {
        fun_console_error_line("Failed to allocate memory");
        return 1;
    }
    Memory buffer = mem_result.value;
    
    // STEP 2: List directory
    ErrorResult result = fun_filesystem_list_directory("/tmp", buffer);
    
    if (fun_error_is_error(result)) {
        if (result.code == ERROR_CODE_DIRECTORY_NOT_FOUND) {
            fun_console_error_line("Directory not found");
        } else if (result.code == ERROR_CODE_NOT_DIRECTORY) {
            fun_console_error_line("Path is not a directory");
        } else {
            fun_console_error_line("Failed to list directory");
        }
        fun_memory_free(&buffer);
        return 1;
    }
    
    // STEP 3: Output contents (newline-separated)
    fun_console_write_line("Directory contents:");
    fun_console_write((const char *)buffer);
    fun_console_flush();
    
    // STEP 4: Cleanup
    fun_memory_free(&buffer);
    
    return 0;
}
```

**Output format:**
```
file1.txt
file2.txt
subdir1
subdir2
```

**Key Points:**
- Entries are newline-separated
- Includes `.` and `..` on some platforms
- Buffer must be large enough (4096 recommended minimum)

---

## Task: Parse Directory Listing

Parse newline-separated directory listing.

```c
#include "filesystem/filesystem.h"
#include "memory/memory.h"
#include "console/console.h"
#include "string/string.h"

int parse_directory_listing(void)
{
    // Allocate and list
    MemoryResult mem_result = fun_memory_allocate(4096);
    if (fun_error_is_error(mem_result.error)) {
        return 1;
    }
    
    ErrorResult list_result = fun_filesystem_list_directory("/tmp", mem_result.value);
    if (fun_error_is_error(list_result.error)) {
        fun_memory_free(&mem_result.value);
        return 1;
    }
    
    // Parse entries
    const char *listing = (const char *)mem_result.value;
    const char *ptr = listing;
    
    while (*ptr != '\0') {
        // Find end of this entry
        const char *newline = ptr;
        while (*newline != '\n' && *newline != '\0') {
            newline++;
        }
        
        // Calculate entry length
        size_t entry_len = newline - ptr;
        
        // Skip empty entries and . / ..
        if (entry_len > 0 && entry_len <= 2) {
            if (entry_len == 1 && ptr[0] == '.') {
                ptr = newline + 1;
                continue;
            }
            if (entry_len == 2 && ptr[0] == '.' && ptr[1] == '.') {
                ptr = newline + 1;
                continue;
            }
        }
        
        // Process this entry
        if (entry_len > 0) {
            fun_console_write("Found: ");
            // Write entry (ptr to newline)
            // ... copy to temp buffer or write character by character ...
            fun_console_write_line("");
        }
        
        // Move to next entry
        if (*newline == '\n') {
            ptr = newline + 1;
        } else {
            break;  // End of string
        }
    }
    
    fun_memory_free(&mem_result.value);
    return 0;
}
```

---

## Task: Remove a Directory

Remove an empty directory.

```c
#include "filesystem/filesystem.h"
#include "console/console.h"

int remove_directory_example(void)
{
    // Remove directory (must be empty)
    ErrorResult result = fun_filesystem_remove_directory("/tmp/test/dir");
    
    if (fun_error_is_error(result)) {
        if (result.code == ERROR_CODE_DIRECTORY_NOT_FOUND) {
            fun_console_error_line("Directory not found");
        } else if (result.code == ERROR_CODE_DIRECTORY_NOT_EMPTY) {
            fun_console_error_line("Directory is not empty");
            // Need to remove contents first
        } else if (result.code == ERROR_CODE_NOT_DIRECTORY) {
            fun_console_error_line("Path is not a directory");
        } else {
            fun_console_error_line("Failed to remove directory");
        }
        return 1;
    }
    
    fun_console_write_line("Directory removed successfully");
    return 0;
}
```

**Key Points:**
- Directory MUST be empty
- Returns `ERROR_CODE_DIRECTORY_NOT_EMPTY` if it contains files
- Does NOT recursively delete - remove contents first

---

## Task: Check if Directory Exists

Check whether a directory exists at the specified path.

```c
#include "filesystem/filesystem.h"
#include "console/console.h"

int check_directory_exists_example(void)
{
    // Check if directory exists
    BoolResult exists_result = fun_directory_exists("/tmp/test");
    
    if (fun_error_is_error(exists_result.error)) {
        fun_console_error_line("Error checking directory");
        return 1;
    }
    
    if (exists_result.value) {
        fun_console_write_line("Directory exists");
    } else {
        fun_console_write_line("Directory does not exist");
    }
    
    return 0;
}
```

**Note:** `fun_directory_exists()` is part of the path-type-refactor and add-file-exists-method changes. Until implemented, check by attempting to list and handling the error.

---

## Task: Iterate Over Files in Directory

Complete pattern for iterating over files.

```c
#include "filesystem/filesystem.h"
#include "memory/memory.h"
#include "console/console.h"
#include "file/file.h"

int iterate_files_example(void)
{
    // STEP 1: List directory
    MemoryResult mem_result = fun_memory_allocate(4096);
    if (fun_error_is_error(mem_result.error)) {
        return 1;
    }
    
    ErrorResult list_result = fun_filesystem_list_directory("/tmp/data", mem_result.value);
    if (fun_error_is_error(list_result.error)) {
        fun_memory_free(&mem_result.value);
        return 1;
    }
    
    // STEP 2: Iterate over entries
    const char *listing = (const char *)mem_result.value;
    const char *ptr = listing;
    
    while (*ptr != '\0') {
        // Find end of entry
        const char *newline = ptr;
        while (*newline != '\n' && *newline != '\0') {
            newline++;
        }
        
        size_t entry_len = newline - ptr;
        
        // Skip . and ..
        if (entry_len <= 2) {
            if (*newline == '\n') ptr = newline + 1;
            else break;
            continue;
        }
        
        // STEP 3: Build full path and process file
        // (simplified - in real code, build path properly)
        fun_console_write("Processing: ");
        // ... write entry name ...
        fun_console_write_line("");
        
        // Example: Read each file
        /*
        char full_path[512];
        // Build full path: /tmp/data/ + entry_name
        // Then:
        MemoryResult file_buf = fun_memory_allocate(1024);
        AsyncResult read = fun_read_file_in_memory((Read){
            .file_path = full_path,
            .output = file_buf.value,
            .bytes_to_read = 1024
        });
        fun_async_await(&read);
        fun_memory_free(&file_buf.value);
        */
        
        // Move to next entry
        if (*newline == '\n') {
            ptr = newline + 1;
        } else {
            break;
        }
    }
    
    // STEP 4: Cleanup
    fun_memory_free(&mem_result.value);
    
    return 0;
}
```

---

## Error Handling

Common directory operation error codes:

| Error Code | Meaning | Action |
|------------|---------|--------|
| `ERROR_CODE_NO_ERROR` | Success | Continue |
| `ERROR_CODE_DIRECTORY_NOT_FOUND` | Path doesn't exist | Create or check path |
| `ERROR_CODE_DIRECTORY_EXISTS` | Already exists | Use existing or error |
| `ERROR_CODE_DIRECTORY_NOT_EMPTY` | Has contents | Remove contents first |
| `ERROR_CODE_NOT_DIRECTORY` | Path is a file | Check path type |
| `ERROR_CODE_PERMISSION_DENIED` | No access | Check permissions |
| `ERROR_CODE_PATH_INVALID` | Bad path format | Validate path |

---

## Cleanup Examples

**Always free listing buffers:**

```c
MemoryResult r = fun_memory_allocate(4096);
ErrorResult list = fun_filesystem_list_directory("/tmp", r.value);

if (fun_error_is_error(list.error)) {
    fun_memory_free(&r.value);  // Free on error
    return 1;
}

// ... use listing ...

fun_memory_free(&r.value);  // Free on success
```

---

## See Also

- **[fundamental-file-io.md](fundamental-file-io.md)** - File operations
- **[fundamental-memory.md](fundamental-memory.md)** - Memory management
- **[filesystem/filesystem.h](../../include/filesystem/filesystem.h)** - Complete API reference
