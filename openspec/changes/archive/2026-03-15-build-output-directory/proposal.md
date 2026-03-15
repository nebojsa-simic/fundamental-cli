## Why

When `fun build` runs, the compiled binary is placed in the project root directory, mixing build artifacts with source files. A dedicated output folder keeps the project root clean and makes it easier to gitignore build outputs.

## What Changes

- `fun build` places the compiled binary in a `build/` output directory instead of the project root
- The binary name includes the target platform: `app-windows-amd64.exe` on Windows, `app-linux-amd64` on Linux — matching the GitHub releases naming convention
- Generated build scripts (`build-windows-amd64.bat`, `build-linux-amd64.sh`) are updated to output the binary to `build/app-windows-amd64.exe` / `build/app-linux-amd64` respectively
- The `build/` directory is created automatically if it does not exist
- `fun clean` removes the `build/` directory

## Capabilities

### New Capabilities

- `build-output-directory`: Generated build scripts place the compiled binary in a `build/` subdirectory with a platform-suffixed name, creating the directory if needed

### Modified Capabilities

- `build-generation`: Generated scripts must emit the binary to `build/app-<platform>-<arch>[.exe]` instead of `app[.exe]` in the project root
- `artifact-cleanup`: `fun clean` must remove the `build/` directory in addition to any existing cleanup targets

## Impact

- `src/build/generator.c` — update `-o` flag and `strip`/echo lines to use `build/app-windows-amd64.exe` and `build/app-linux-amd64`
- `src/commands/cmd_clean.c` — clean command must remove `build/`
- Generated project scripts (`build-windows-amd64.bat`, `build-linux-amd64.sh`) — output path and binary name change
- Smoke tests may need updating if they check the binary location or name
