## Context

`fun build` generates platform-specific build scripts (`build-windows-amd64.bat`, `build-linux-amd64.sh`) and executes them. Both scripts currently pass `-o app.exe` / `-o app` to GCC, placing the binary in the project root. The `cmd_clean.c` command tries to delete `app.exe` / `app` from the root.

All binary output path strings are hardcoded as string literals inside `build_generate_windows()` and `build_generate_linux()` in `src/build/generator.c`.

## Goals / Non-Goals

**Goals:**
- Generated build scripts emit the binary to `build/app-windows-amd64.exe` (Windows) or `build/app-linux-amd64` (Linux)
- Binary name includes platform and architecture suffix — matching GitHub releases convention
- Scripts create the `build/` directory before compiling (mkdir step)
- `fun clean` removes the `build/` directory rather than individual root-level files
- The change is self-contained: no new abstractions, no configuration

**Non-Goals:**
- Making the output directory or binary name configurable
- Updating existing pre-generated scripts in user projects (they regenerate on next `fun build --clean`)
- Deriving the binary base name from the project folder/config (separate concern — hardcode `app` for now)

## Decisions

**Decision: Hardcode `build/` as the output directory**

Alternative considered: read the output path from a config file or `--output` flag. Rejected — over-engineering for the current need. A single well-known folder (`build/`) matches common C project conventions and is simple to gitignore.

**Decision: Add mkdir step directly in the generated script**

The generated scripts are standalone batch/shell files. Adding `if not exist build mkdir build` (Windows) or `mkdir -p build` (Linux) at the top of the gcc section keeps the scripts self-contained without requiring `fun build` to pre-create the directory via the fundamental filesystem API.

**Decision: `fun clean` removes the entire `build/` directory**

The current `cmd_clean.c` deletes `app.exe` / `app` by filename. After this change the binary lives in `build/`, so clean should remove the whole `build/` directory. This is simpler and more thorough than hunting for individual files. Use platform-native `rmdir /s /q build` (Windows) or `rm -rf build` (Linux) via `fun_process_spawn`.

## Risks / Trade-offs

- **Existing generated scripts are stale**: If a user has a pre-existing `build-windows-amd64.bat` still pointing to `-o app.exe`, `fun build` will reuse it (build_detect finds it as present) and output to the root as before. → Mitigation: document that users should delete old scripts or use `fun build --clean` to regenerate. This is acceptable because the detection/regeneration flow already handles the "missing script" case.
- **`build/` conflicts with build script names**: The directory `build/` and the script filenames `build-windows-amd64.bat` do not conflict (one is a directory, the others are files in the root).

## Migration Plan

1. Modify `src/build/generator.c`: update the string literals in `build_generate_windows()` and `build_generate_linux()` to add a mkdir step and change `-o` target to `build/app-windows-amd64.exe` / `build/app-linux-amd64`. Also update `strip` and echo lines.
2. Modify `src/commands/cmd_clean.c`: replace individual `remove_file("app.exe")` with a recursive directory removal of `build/`.
3. Rebuild `fun.exe` and run the smoke test to verify end-to-end.

Rollback: revert the two source files. No data migration needed.
