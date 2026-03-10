## ADDED Requirements

### Requirement: System scans for C source files
The system SHALL recursively discover `.c` files in `src/` directory.

#### Scenario: Find source files
- **WHEN** scanning for sources
- **THEN** all `src/**/*.c` files are discovered

#### Scenario: Handle empty src
- **WHEN** `src/` contains no `.c` files
- **THEN** build script uses only entry point

#### Scenario: Include command files
- **WHEN** `commands/*.c` files exist
- **THEN** they are included in build
