## MODIFIED Requirements

### Requirement: System scans for C source files
The system SHALL recursively discover `.c` files in `src/` and `arch/<platform>/` directories, and separately resolve vendor `.c` files via the overlay system.

#### Scenario: Find source files in src/
- **WHEN** scanning for sources
- **THEN** all `src/**/*.c` files are discovered via real directory traversal

#### Scenario: Find arch files for current platform
- **WHEN** scanning for sources on windows-amd64
- **THEN** all `arch/build/windows-amd64/*.c` files are discovered and included

#### Scenario: Handle empty src
- **WHEN** `src/` contains no `.c` files
- **THEN** build script uses only entry point and vendor-resolved files

#### Scenario: Include command files
- **WHEN** `commands/*.c` files exist
- **THEN** they are included in build

#### Scenario: Discover nested subdirectories
- **WHEN** `src/` contains subdirectories (e.g., `src/build/`, `src/commands/`, `src/test/`)
- **THEN** all `.c` files in all subdirectories are discovered and included

#### Scenario: Vendor files excluded from src scan
- **WHEN** scanning `src/` and `arch/<platform>/`
- **THEN** `vendor/` directory contents are not included in the src scan result; vendor files come exclusively from overlay resolution
