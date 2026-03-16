## ADDED Requirements

### Requirement: System generates build scripts when missing
The system SHALL create platform-specific build scripts by scanning source files.

#### Scenario: Scan source files
- **WHEN** generating build script
- **THEN** system recursively scans `src/**/*.c` for source files

#### Scenario: Generate Windows batch file
- **WHEN** build script is needed on Windows
- **THEN** `build-windows-amd64.bat` is created with GCC commands that output the binary to `build/app-windows-amd64.exe`

#### Scenario: Generate Linux shell script
- **WHEN** build script is needed on Linux
- **THEN** `build-linux-amd64.sh` is created with GCC commands that output the binary to `build/app-linux-amd64`

### Requirement: Generated scripts create output directory
The system SHALL ensure generated build scripts create the `build/` output directory before compiling.

#### Scenario: Windows script creates build directory
- **WHEN** `build-windows-amd64.bat` is executed
- **THEN** the script runs `if not exist build mkdir build` before the GCC command

#### Scenario: Linux script creates build directory
- **WHEN** `build-linux-amd64.sh` is executed
- **THEN** the script runs `mkdir -p build` before the GCC command

### Requirement: Generated scripts include vendor paths
Build scripts SHALL include proper paths to vendored fundamental library.

#### Scenario: Include vendor fundamental
- **WHEN** build script is generated
- **THEN** it includes `-I vendor/fundamental/include`

#### Scenario: Link vendor sources
- **WHEN** build script is generated
- **THEN** it includes startup, platform, async, process, file, filesystem, console, string, and array fundamental source and arch files
