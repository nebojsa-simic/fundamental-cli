## MODIFIED Requirements

### Requirement: System scans for C source files
The system SHALL recursively discover `.c` files in `src/` directory using filesystem traversal.

#### Scenario: Find source files
- **WHEN** scanning for sources
- **THEN** all `src/**/*.c` files are discovered via real directory traversal (not a hardcoded list)

#### Scenario: Handle empty src
- **WHEN** `src/` contains no `.c` files
- **THEN** build script uses only entry point

#### Scenario: Include command files
- **WHEN** `commands/*.c` files exist
- **THEN** they are included in build

#### Scenario: Discover nested subdirectories
- **WHEN** `src/` contains subdirectories (e.g., `src/build/`, `src/commands/`, `src/test/`)
- **THEN** all `.c` files in all subdirectories are discovered and included
