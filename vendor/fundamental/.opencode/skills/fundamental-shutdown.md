---
name: fundamental-shutdown
description: Graceful shutdown handling with cleanup handlers for Fundamental Library
license: MIT
compatibility: Complements fundamental-expert skill
metadata:
  author: fundamental-library
  version: "1.0"
  category: lifecycle-management
  related: fundamental-platform, fundamental-console
---

# Fundamental Library - Shutdown Framework Skill

I provide copy-paste examples for graceful shutdown handling using the Fundamental Library shutdown framework.

---

## Quick Reference

| Task | Function | Example |
|------|----------|---------|
| Register cleanup | `shutdown_register_cleanup()` | See below |
| Run shutdown | `fun_shutdown_run()` | Called by signal handlers |
| Shutdown phases | `SHUTDOWN_PHASE_*` | APP=99, NETWORK=5, CONFIG=4, FILESYSTEM=3, MEMORY=2, PLATFORM=1 |

**See Also:** [fundamental-platform](fundamental-platform.md) for initialization, [fundamental-console](fundamental-console.md) for cleanup output

---

## Task: Register Application Cleanup Handler

Register a cleanup function that runs when the program receives Ctrl+C or SIGTERM.

```c
#include "fundamental/shutdown/shutdown.h"
#include "fundamental/console/console.h"
#include "fundamental/file/file.h"
#include "fundamental/string/string.h"
#include "fundamental/async/async.h"

/* Cleanup function - called during shutdown */
static void app_cleanup(void)
{
    const char *content = "Application cleanup completed\n";
    
    Write params = {
        .file_path = "shutdown.log",
        .input = (Memory)content,
        .bytes_to_write = fun_string_length(content),
        .durability_mode = FILE_DURABILITY_SYNC
    };
    
    AsyncResult result = fun_write_memory_to_file(params);
    fun_async_await(&result, -1);
    
    if (fun_error_is_ok(result.error)) {
        fun_console_write_line("Cleanup: Log file written");
    }
}

int main(void)
{
    /* STEP 1: Register cleanup BEFORE fun_startup_run() */
    shutdown_register_cleanup(SHUTDOWN_PHASE_APP, app_cleanup);
    
    /* STEP 2: Initialize startup (installs signal handlers) */
    fun_startup_run();
    
    /* STEP 3: Run application logic */
    fun_console_write_line("Application running...");
    fun_console_write_line("Press Ctrl+C to trigger graceful shutdown");
    
    /* Application logic here */
    
    return 0;
}
```

**Key Points:**
- Register cleanup **before** `fun_startup_run()` - signal handlers installed during startup
- Cleanup functions execute in **reverse phase order** (APP first, PLATFORM last)
- Maximum 32 cleanup functions can be registered
- Handlers should be fast and non-blocking

---

## Task: Multiple Cleanup Handlers

Register multiple cleanup functions at different phases.

```c
#include "fundamental/shutdown/shutdown.h"

/* Network cleanup - phase 5 */
static void network_cleanup(void)
{
    /* Close sockets, release network resources */
    fun_console_write_line("Network: Closing connections...");
}

/* Config cleanup - phase 4 */
static void config_cleanup(void)
{
    /* Free config memory, flush settings */
    fun_console_write_line("Config: Releasing configuration...");
}

/* Filesystem cleanup - phase 3 */
static void filesystem_cleanup(void)
{
    /* Flush file buffers, close handles */
    fun_console_write_line("Filesystem: Flushing buffers...");
}

/* Application cleanup - phase 99 (runs first) */
static void app_cleanup(void)
{
    fun_console_write_line("App: Saving state...");
}

int main(void)
{
    /* Register in reverse order of desired execution */
    shutdown_register_cleanup(SHUTDOWN_PHASE_APP, app_cleanup);
    shutdown_register_cleanup(SHUTDOWN_PHASE_FILESYSTEM, filesystem_cleanup);
    shutdown_register_cleanup(SHUTDOWN_PHASE_CONFIG, config_cleanup);
    shutdown_register_cleanup(SHUTDOWN_PHASE_NETWORK, network_cleanup);
    
    fun_startup_run();
    
    /* Application runs... */
    
    return 0;
}

/* Shutdown executes: APP(99) → NETWORK(5) → CONFIG(4) → FILESYSTEM(3) */
```

**Execution Order:**
1. `app_cleanup()` - phase 99 (first)
2. `network_cleanup()` - phase 5
3. `config_cleanup()` - phase 4
4. `filesystem_cleanup()` - phase 3 (last of these)

---

## Task: Shutdown with File I/O

Write files during cleanup to save state or logs.

```c
#include "fundamental/shutdown/shutdown.h"
#include "fundamental/file/file.h"
#include "fundamental/string/string.h"
#include "fundamental/async/async.h"

static void save_state_cleanup(void)
{
    const char *state = "application_state_data";
    
    Write params = {
        .file_path = "state.dat",
        .input = (Memory)state,
        .bytes_to_write = fun_string_length(state),
        .durability_mode = FILE_DURABILITY_SYNC  /* Ensure data on disk */
    };
    
    AsyncResult result = fun_write_memory_to_file(params);
    fun_async_await(&result, -1);
    
    if (fun_error_is_ok(result.error)) {
        fun_console_write_line("State saved successfully");
    } else {
        fun_console_error_line("Failed to save state");
    }
}

int main(void)
{
    shutdown_register_cleanup(SHUTDOWN_PHASE_APP, save_state_cleanup);
    fun_startup_run();
    
    /* Application runs... */
    
    return 0;
}
```

**Important:** Use `FILE_DURABILITY_SYNC` or `FILE_DURABILITY_FULL` in cleanup to ensure data reaches disk before process exits.

---

## Task: Handle Shutdown Types

Check shutdown type in cleanup handler for conditional cleanup.

```c
#include "fundamental/shutdown/shutdown.h"
#include "fundamental/console/console.h"

static fun_shutdown_type g_shutdown_type = SHUTDOWN_NORMAL;

static void conditional_cleanup(void)
{
    switch (g_shutdown_type) {
    case SHUTDOWN_NORMAL:
        fun_console_write_line("Normal exit - minimal cleanup");
        break;
        
    case SHUTDOWN_EXTERNAL:
        fun_console_write_line("External signal - save state");
        /* Save application state */
        break;
        
    case SHUTDOWN_EMERGENCY:
        fun_console_write_line("Emergency - quick cleanup only");
        /* Only essential cleanup */
        break;
        
    default:
        break;
    }
}

int main(void)
{
    shutdown_register_cleanup(SHUTDOWN_PHASE_APP, conditional_cleanup);
    fun_startup_run();
    
    /* Application runs... */
    
    return 0;
}
```

**Shutdown Types:**
- `SHUTDOWN_NORMAL` (0) - Normal program exit
- `SHUTDOWN_ABNORMAL` (1) - Crash or error
- `SHUTDOWN_EXTERNAL` (2) - Ctrl+C, SIGTERM
- `SHUTDOWN_EMERGENCY` (3) - System shutdown/logoff

---

## Task: Platform-Specific Signal Handling

Install custom signal handlers for additional control.

```c
#include "fundamental/shutdown/shutdown.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

/* Custom Windows console handler */
static BOOL WINAPI custom_ctrl_handler(DWORD ctrl_type)
{
    switch (ctrl_type) {
    case CTRL_C_EVENT:
        fun_console_write_line("Custom: Ctrl+C detected");
        fun_shutdown_run(SHUTDOWN_EXTERNAL, 130);
        return TRUE;
        
    case CTRL_CLOSE_EVENT:
        fun_console_write_line("Custom: Console closing");
        fun_shutdown_run(SHUTDOWN_EXTERNAL, 0);
        return TRUE;
        
    default:
        return FALSE;
    }
}

int main(void)
{
    /* Register cleanup */
    shutdown_register_cleanup(SHUTDOWN_PHASE_APP, app_cleanup);
    
    /* Install custom handler instead of default */
    SetConsoleCtrlHandler(custom_ctrl_handler, TRUE);
    
    /* Application runs... */
    
    return 0;
}
```

---

## Common Pitfalls

### ❌ Wrong: Register After Startup

```c
fun_startup_run();  // Signal handlers installed
shutdown_register_cleanup(SHUTDOWN_PHASE_APP, app_cleanup);  // Too late!
```

**Fix:** Register **before** `fun_startup_run()`:
```c
shutdown_register_cleanup(SHUTDOWN_PHASE_APP, app_cleanup);
fun_startup_run();
```

### ❌ Wrong: Slow Cleanup Handler

```c
static void slow_cleanup(void)
{
    /* Don't do this - takes too long */
    for (int i = 0; i < 1000000; i++) {
        /* Long computation */
    }
}
```

**Fix:** Keep cleanup fast:
```c
static void fast_cleanup(void)
{
    /* Quick operations only */
    fun_console_write_line("Cleanup done");
}
```

### ❌ Wrong: Allocate Memory in Cleanup

```c
static void bad_cleanup(void)
{
    MemoryResult result = fun_memory_allocate(1024);  // Risky during shutdown
    /* ... */
}
```

**Fix:** Use pre-allocated resources or stack memory:
```c
static void good_cleanup(void)
{
    char buffer[256];  /* Stack allocation - safe */
    /* ... */
}
```

---

## Testing Shutdown

### Manual Test

```bash
cd demos/shutdown
./build-windows-amd64.bat  # or ./build-linux-amd64.sh
./demo.exe
# Press Ctrl+C during countdown
type shutdown_cleanup.txt  # Verify cleanup ran
```

### Automated Test

```c
#include "fundamental/shutdown/shutdown.h"

static int cleanup_ran = 0;

static void test_cleanup(void)
{
    cleanup_ran = 1;
}

void test_shutdown_registration(void)
{
    shutdown_register_cleanup(SHUTDOWN_PHASE_APP, test_cleanup);
    /* Verify registration succeeded (check internal state) */
}
```

---

## See Also

- `include/fundamental/shutdown/shutdown.h` - API header with full documentation
- `src/shutdown/shutdown.c` - Core implementation
- `arch/signals/*/signal.c` - Platform-specific signal handlers
- `demos/shutdown/` - Working demo with Ctrl+C handling
- [fundamental-platform](fundamental-platform.md) - Startup initialization
- [fundamental-file-io](fundamental-file-io.md) - File operations in cleanup
