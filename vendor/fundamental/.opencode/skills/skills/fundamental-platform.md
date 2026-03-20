---
name: fundamental-platform
description: Platform detection with Fundamental Library - OS and architecture via arch layer
license: MIT
compatibility: Complements fundamental-expert skill
metadata:
  author: fundamental-library
  version: "1.1"
  category: platform-detection
  related: fundamental-async, fundamental-console
---

# Fundamental Library - Platform Detection Skill

I provide copy-paste examples for platform detection using the Fundamental Library.

---

## Quick Reference

| Task | Function | Signature |
|------|----------|-----------|
| Get OS + arch | `fun_platform_get()` | `void fun_platform_get(OutputPlatform platform)` |
| OS to string | `fun_platform_os_to_string()` | `ErrorResult fun_platform_os_to_string(PlatformOS os, OutputString platformOsResult)` |
| Arch to string | `fun_platform_arch_to_string()` | `ErrorResult fun_platform_arch_to_string(PlatformArch arch, OutputString platformArchResult)` |
| Full string | `fun_platform_to_string()` | `CanReturnError(void) fun_platform_to_string(Platform platform, OutputString output)` |

**OS Values:**
- `PLATFORM_OS_WINDOWS`
- `PLATFORM_OS_LINUX`
- `PLATFORM_OS_DARWIN`
- `PLATFORM_OS_UNKNOWN`

**Arch Values:**
- `PLATFORM_ARCH_AMD64`
- `PLATFORM_ARCH_ARM64`
- `PLATFORM_ARCH_UNKNOWN`

---

## Task: Get Current Platform

Detect OS and architecture. Values are constant — provided by the arch layer at link time.

```c
#include "platform/platform.h"

void platform_detect_example(void)
{
    Platform p;
    fun_platform_get(&p);

    if (p.os == PLATFORM_OS_WINDOWS) {
        // Windows-specific code
    } else if (p.os == PLATFORM_OS_LINUX) {
        // Linux-specific code
    } else if (p.os == PLATFORM_OS_DARWIN) {
        // macOS-specific code
    }

    if (p.arch == PLATFORM_ARCH_AMD64) {
        // x86-64 code
    } else if (p.arch == PLATFORM_ARCH_ARM64) {
        // ARM64 code
    }
}
```

**Key Points:**
- Cannot fail — simple constant lookup, no error to check
- Always pass a valid `OutputPlatform` pointer; NULL is not accepted

---

## Task: Convert OS/Arch to String

Write human-readable names into caller-provided buffers.

```c
#include "platform/platform.h"
#include "console/console.h"

void platform_names_example(void)
{
    Platform p;
    fun_platform_get(&p);

    char os_buf[16];
    char arch_buf[16];

    fun_platform_os_to_string(p.os, os_buf);     // "windows", "linux", "darwin"
    fun_platform_arch_to_string(p.arch, arch_buf); // "amd64", "arm64"

    fun_console_write((String)os_buf);
    fun_console_write("-");
    fun_console_write_line((String)arch_buf);
}
```

**Key Points:**
- OS buffer must be at least 16 bytes
- Arch buffer must be at least 16 bytes
- Only fails if buffer is NULL

---

## Task: Get Full Platform String

Write the combined `"os-arch"` string into a caller-provided buffer.

```c
#include "platform/platform.h"
#include "console/console.h"

void platform_to_string_example(void)
{
    Platform p;
    fun_platform_get(&p);

    char buf[32];  // must be at least 32 bytes
    voidResult res = fun_platform_to_string(p, buf);

    if (fun_error_is_error(res.error)) {
        // Only fails if buf is NULL
        return;
    }

    fun_console_write_line((String)buf);  // e.g. "windows-amd64"
}
```

**Key Points:**
- Buffer must be at least 32 bytes
- Only error case is NULL buffer
- Format is always `"<os>-<arch>"`

---

## Common Patterns

### Select Script by Platform
```c
Platform p;
fun_platform_get(&p);

const char *script = (p.os == PLATFORM_OS_WINDOWS)
    ? "build-windows-amd64.bat"
    : "build-linux-amd64.sh";
```

### Branch on Both OS and Arch
```c
Platform p;
fun_platform_get(&p);

if (p.os == PLATFORM_OS_WINDOWS && p.arch == PLATFORM_ARCH_AMD64) {
    // windows/amd64 path
} else if (p.os == PLATFORM_OS_LINUX && p.arch == PLATFORM_ARCH_ARM64) {
    // linux/arm64 path
} else {
    // fallback
}
```

### Log Platform at Startup
```c
Platform p;
fun_platform_get(&p);

char buf[32];
fun_platform_to_string(p, buf);
fun_console_write("Platform: ");
fun_console_write_line((String)buf);
```

---

## See Also

- **[fundamental-console.md](fundamental-console.md)** - Writing platform info to output
- **[fundamental-async.md](fundamental-async.md)** - Spawning platform-specific processes
- **[include/platform/platform.h](../../include/platform/platform.h)** - Complete platform API
