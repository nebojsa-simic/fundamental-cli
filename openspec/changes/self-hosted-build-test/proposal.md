## Why

`fun build` was designed to build projects that use the fundamental library, yet `fun.exe` itself is built and tested with hand-maintained batch files that drift from what `fun build` produces. Dogfooding closes this gap: it validates `fun build` on a real complex project, forces improvements to the generator, and eliminates the maintenance burden of the hand-crafted scripts.

## What Changes

- `fun build` gains real recursive directory scanning — replacing the fake hardcoded file list with actual `FindFirstFile`/`FindNextFile` (Windows) traversal
- The generated binary name comes from the `name` field in `fun.ini` instead of the hardcoded `app`
- The generator includes all fundamental vendor modules found in `vendor/fundamental/` rather than the minimal subset — ensuring complex projects like `fun.exe` itself compile correctly
- `fun.ini` in the repo is updated to correctly describe the `fun` project (`name = fun`)
- The `build-windows-amd64.bat` becomes a one-time bootstrap (builds `fun.exe` from scratch); subsequent builds use `fun build`
- The smoke test bootstraps with `build-windows-amd64.bat`, then validates `fun build` rebuilds `fun.exe` correctly

## Capabilities

### New Capabilities

- `fun-ini-project-name`: Generator reads `name` from `fun.ini` to derive the output binary name (`<name>-windows-amd64.exe` / `<name>-linux-amd64`)

### Modified Capabilities

- `source-scanning`: Real recursive OS-level directory scan replacing the hardcoded file list — the spec already requires `src/**/*.c` discovery; the implementation must now deliver it
- `build-generation`: Generator includes all vendor fundamental modules present in `vendor/fundamental/` (not just the minimal console/string/memory subset)

## Impact

- `src/build/generator.c` — replace `scan_directory_recursive()` stub with real `FindFirstFile`/`FindNextFile` (Windows) + `opendir`/`readdir` (Linux) recursion; extend vendor module list to include async, process, file, filesystem, array, platform
- `fun.ini` in repo root — set `name = fun`
- `smoke-test-windows-amd64.bat` — after bootstrap build, add a step that deletes the generated script and re-runs `fun build` to verify self-hosted rebuild produces `build/fun-windows-amd64.exe`
