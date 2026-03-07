# Fundamental Memory Module

## Overview

The Fundamental Memory Module is a cross-platform implementation of basic memory management functions for both Windows and Linux systems. It provides a set of functions for allocating, reallocating, freeing, and manipulating memory without relying on the standard C library. This module uses platform-specific system calls for memory operations, offering fine-grained control over memory management.

## Features

- Cross-platform support (Windows and Linux)
- Memory allocation and deallocation
- Memory reallocation
- Memory filling
- Memory size querying
- Error handling with detailed error codes and messages

## API

### `memoryAllocate(size_t size)`

Allocates a block of memory of the specified size.

- If `size` is 0, a minimum-sized memory block is allocated.
- Returns a valid pointer on success, or NULL on failure.

### `memoryReallocate(Memory memory, size_t newSize)`

Reallocates a memory block to a new size.

- If `memory` is NULL, the function returns NULL without performing any allocation.
- If `newSize` is 0, the function frees the memory and returns NULL.

### `memoryFree(Memory* memory)`

Frees a previously allocated memory block.

- Safe to call with NULL pointer.
- Multiple calls with the same pointer (double free) do not result in an error.

### `memoryFill(Memory memory, size_t size, uint64_t value)`

Fills a memory block with a specified value.

### `memorySize(Memory memory)`

Returns the size of an allocated memory block.

- Note: On Linux, this function returns an approximation due to system call limitations.

## Error Handling

All functions return a result type that includes both the operation's result and an error code with a message. Check the `Error` field of the result to determine if an operation was successful.

## Key Differences from Standard Library

1. **Zero-size allocation**: `memoryAllocate(0)` returns a valid pointer to a minimum-sized allocation, not NULL.
2. **Reallocation of NULL**: `memoryReallocate(NULL, size)` returns NULL, unlike `realloc` which behaves like `malloc` in this case.
3. **Double free safety**: Calling `memoryFree` multiple times on the same pointer does not result in an error.
4. **Platform-specific implementations**: Uses Windows API (HeapAlloc, etc.) on Windows and direct system calls on Linux.

## Platform-Specific Details

### Windows
- Uses Windows API functions (HeapAlloc, HeapReAlloc, HeapFree, HeapSize)
- Provides accurate memory size information

### Linux
- Uses direct system calls (mmap, munmap, brk)
- **Supports only AMD64 (x86-64) architecture**
- Memory size function returns an approximation (typically page size)
- Implemented using inline assembly specific to AMD64 system call conventions

## Usage

Include the `memory.h` header in your project and link against the Fundamental library.

```C
#include "memory.h" 

int main() {
    MemoryResult result = memoryAllocate(1024);
    if (fun_error_is_ok(result.error)) {
        // Use the allocated memory

        // And free it
        memoryFree(&result.value);
    }
    return 0;
}
```

## Building

TODO

## Testing

Running tests require that GCC is installed on the system.

Linux:
```sh
cd tests/memory
./build-linux-amd64.sh
./test
```

Windows:
```powershell
cd tests\memory
.\build-windows-amd64.bat
.\test
```

## Contributing

See [CONTRIBUTING.md](../CONTRIBUTING.md).

## License

This project is licensed under the MIT License - see the [LICENSE](../LICENSE) file for details.
