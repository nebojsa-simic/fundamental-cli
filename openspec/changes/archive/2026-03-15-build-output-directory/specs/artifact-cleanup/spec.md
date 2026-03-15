## MODIFIED Requirements

### Requirement: System removes build artifacts
The system SHALL clean compiled binaries and temporary files with `fun clean`.

#### Scenario: Remove build directory on Windows
- **WHEN** `fun clean` is run on Windows
- **THEN** the `build/` directory and all its contents are removed

#### Scenario: Remove build directory on Linux
- **WHEN** `fun clean` is run on Linux
- **THEN** the `build/` directory and all its contents are removed

#### Scenario: Remove object files
- **WHEN** `fun clean` is run
- **THEN** `.o` files are removed
