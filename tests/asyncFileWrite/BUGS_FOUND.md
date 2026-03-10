# Bugs Found in Vendored Fundamental Library

## Bug 1: Function Signature Mismatch (CRITICAL)

**Location:** `vendor/fundamental/include/filesystem/filesystem.h` vs `vendor/fundamental/src/filesystem/directory.c`

**Problem:**
- **Header (line 46):** `ErrorResult fun_filesystem_create_directory(String path);`
- **Implementation (line 130):** `ErrorResult fun_filesystem_create_directory(const char *path)`

**Impact:** Linker cannot find the function - signature mismatch causes different symbol names or compiler treats them as different functions.

**Fix:**
```c
// Change vendor/fundamental/include/filesystem/filesystem.h line 46 from:
ErrorResult fun_filesystem_create_directory(String path);
// To:
ErrorResult fun_filesystem_create_directory(const char *path);
```

---

## Bug 2: Missing fun_memory_copy Implementation

**Location:** Declared in `vendor/fundamental/include/memory/memory.h` but not implemented

**Problem:** Function is declared and used throughout the codebase but has no implementation.

**Impact:** Linker errors when fileWriteMmap.c tries to use fun_memory_copy.

**Fix:** Implement fun_memory_copy in vendor/fundamental/arch/memory/windows-amd64/memory.c or vendor/fundamental/src/memory/*.c

---

## Bug 3: cmd_init.c Missing from Repository

**Location:** File was never committed to git

**Problem:** The implementation exists locally but is not in version control.

**Impact:** Build fails because cmd_init.c doesn't exist.

**Fix:** Commit the working cmd_init.c file or recreate it.

---

## Test Results

✅ **Large template async file write test PASSES**
- Location: `tests/asyncFileWrite/test_large.c`
- Writes files up to ~1500 bytes successfully
- Multiple sequential writes work

❌ **cmd_init.c fails**
- Due to Bug #1 (signature mismatch)
- Cannot link against fun_filesystem_create_directory

---

## Summary

The original "hang" was actually a LINK ERROR, not a runtime hang. The async file write itself works fine (proven by test_large.c). The bugs preventing cmd_init.c from working are:

1. **Function signature mismatch** - prevents linking
2. **Missing file** - cmd_init.c not in repo

Once these are fixed, the async file write will work correctly.
