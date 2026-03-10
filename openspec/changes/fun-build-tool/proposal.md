## Why

Developers need a unified build command that works across platforms without managing platform-specific build scripts manually. Currently, each project requires separate batch files and shell scripts, and there's no standard way to detect, generate, or execute builds.

## What Changes

- **New**: `fun build` command to compile projects for current platform
- **New**: Automatic platform detection (Windows vs Linux/macOS)
- **New**: Build script detection and execution (existing scripts take priority)
- **New**: Build script generation when no existing scripts found (scans `src/**/*.c`)
- **New**: Build flags support (`--release`, `--clean`, `--verbose`)
- **New**: `fun clean` command to remove build artifacts
- **Modified**: Existing build workflows now optional - fun can generate and manage build scripts

## Capabilities

### New Capabilities

- `platform-detection`: Automatic OS and architecture detection for build targeting
- `build-detection`: Discovery of existing build scripts or determination that generation is needed
- `build-generation`: Dynamic build script creation based on source file scanning
- `build-execution`: Running platform-specific build scripts with proper error handling
- `artifact-cleanup`: Removal of compiled binaries, object files, and temporary build artifacts
- `source-scanning`: Recursive discovery of `.c` files in `src/` directory for build configuration

### Modified Capabilities

<!-- No existing capabilities modified -->

## Impact

- **Code**: New modules in `src/build/detector.c/h`, `src/build/generator.c/h`, `src/build/executor.c/h`
- **Commands**: `src/commands/cmd_build.c/h`, `src/commands/cmd_clean.c/h`
- **Templates**: Embedded build script templates for Windows batch and Linux shell
- **fun.ini**: Reads `[build]` section for compiler flags and entry point configuration
