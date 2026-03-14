## ADDED Requirements

### Requirement: System generates build scripts when missing
The system SHALL create platform-specific build scripts by scanning source files.

#### Scenario: Scan source files
- **WHEN** generating build script
- **THEN** system recursively scans `src/**/*.c` for source files

#### Scenario: Generate Windows batch file
- **WHEN** build script is needed on Windows
- **THEN** `build-windows-amd64.bat` is created with GCC commands

#### Scenario: Generate Linux shell script
- **WHEN** build script is needed on Linux
- **THEN** `build-linux-amd64.sh` is created with GCC commands

### Requirement: Generated scripts include vendor paths
Build scripts SHALL include proper paths to vendored fundamental library.

#### Scenario: Include vendor fundamental
- **WHEN** build script is generated
- **THEN** it includes `-I vendor/fundamental/include`

#### Scenario: Link vendor sources
- **WHEN** build script is generated
- **THEN** it includes fundamental source and arch files
