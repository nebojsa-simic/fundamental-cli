## Context

`fun build` has two stubbed implementations that prevent it from building anything non-trivial, including itself:

1. **Fake source scanner** — `scan_directory_recursive()` in `generator.c` checks a hardcoded list of ~10 files instead of traversing the filesystem. The `source-scanning` spec says it SHALL discover `src/**/*.c` but the code doesn't.
2. **Minimal vendor modules** — the generated script includes only console/string/memory. `fun.exe` itself requires async, process, file, filesystem, platform, array — none of which are generated.

Additionally, the output binary name is hardcoded as `app`; there is no `fun.ini` in the repo root, and the smoke test never exercises `fun build` rebuilding `fun.exe`.

## Goals / Non-Goals

**Goals:**
- Real recursive `src/` scanning using `fun_filesystem_list_directory`
- Binary name derived from `name` field in `fun.ini`
- Generated scripts include a comprehensive set of fundamental vendor modules so complex projects (including `fun.exe` itself) compile
- `fun.ini` added to repo root describing the `fun` project
- Smoke test validates `fun build` can rebuild `fun.exe`

**Non-Goals:**
- Auto-detecting which vendor modules a project needs from `#include` analysis
- Network module inclusion (requires extra platform libs, opt-in only)
- `fun test` integration (separate concern — `fun test` already works; the smoke test already calls it)
- Cross-compilation or multi-platform builds from a single host

## Decisions

**Decision: Use `fun_filesystem_list_directory` for scanning, not OS-native APIs**

`fun_filesystem_list_directory` returns newline-separated entry names for a path and is already platform-abstracted. The alternative (using `FindFirstFile`/`FindNextFile` directly in `generator.c`) would re-implement what the filesystem module already provides and break the no-OS-calls-outside-arch rule. Using the library function is the correct approach.

The scanner walks entries from `fun_filesystem_list_directory`, checks if each ends in `.c` (file, include it), or recurse if the entry is a subdirectory. Uses `fun_path_join` to build child paths.

**Decision: Comprehensive vendor module list hardcoded in generator, not from `fun.ini`**

Options considered:
- A. `fun.ini` `[modules]` section — flexible but requires config parsing and more work
- B. Always include all vendor modules in the generated script — simplest, slightly larger binary
- C. Hardcode a "comprehensive" set (everything except network/stream/hashmap/rbtree/set) — good balance

Option C wins for now: the comprehensive set (startup, platform, async, process, file, filesystem, console, string, array) covers all real projects. Network and collection modules are opt-in concerns for a future change. This gets dogfooding working without over-engineering.

**Decision: Read binary name from `fun.ini` using simple line parsing**

The config module (`vendor/fundamental/src/config/`) exists but may pull in dependencies. Instead, `generator.c` reads `fun.ini` with a minimal line-by-line scan: find the line starting with `name =`, extract the value. This avoids a new dependency and keeps the generator self-contained. Fall back to `app` if `fun.ini` is absent or has no `name` field.

## Risks / Trade-offs

- **Scan buffer sizing**: `fun_filesystem_list_directory` fills a caller-allocated buffer. A large `src/` tree could overflow a small buffer. → Mitigation: allocate 16KB for directory listings; cap recursion depth at 8 levels.
- **Generated script size growth**: Including the comprehensive vendor set adds ~20 source files to every generated script, increasing script length. → Acceptable — the script still fits within the static 4096-byte buffer only if we increase it. Buffer must grow to ~8192 bytes.
- **Chicken-and-egg**: The first build of `fun.exe` from a clean checkout still needs `build-windows-amd64.bat`. → `build-windows-amd64.bat` stays as the bootstrap; the dogfooding is validated by the smoke test deleting the generated script and re-running `fun build`.

## Migration Plan

1. Increase `script_buffer` in `generator.c` from 4096 to 8192 bytes.
2. Implement real `scan_directory_recursive()` using `fun_filesystem_list_directory` + `fun_path_join` + `fun_directory_exists`.
3. Add `fun_ini_read_name()` helper in `generator.c` that reads `fun.ini` and returns the `name` value.
4. Update `build_generate_windows()` and `build_generate_linux()` to call `fun_ini_read_name()` for the output binary name and include the comprehensive vendor module list.
5. Create `fun.ini` in the repo root with `name = fun`.
6. Update `smoke-test-windows-amd64.bat` to add a step: delete the generated `build-windows-amd64.bat` from the smoke project, re-run `fun build`, verify `build/fun-windows-amd64.exe` exists and runs.
