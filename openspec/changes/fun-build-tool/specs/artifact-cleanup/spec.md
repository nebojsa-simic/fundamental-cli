## ADDED Requirements

### Requirement: System removes build artifacts
The system SHALL clean compiled binaries and temporary files with `fun clean`.

#### Scenario: Remove Windows binary
- **WHEN** `fun clean` is run on Windows
- **THEN** `.exe` files are removed

#### Scenario: Remove Linux binary
- **WHEN** `fun clean` is run on Linux
- **THEN** compiled binaries without extension are removed

#### Scenario: Remove object files
- **WHEN** `fun clean` is run
- **THEN** `.o` files are removed
