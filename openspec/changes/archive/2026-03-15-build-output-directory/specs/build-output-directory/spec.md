## ADDED Requirements

### Requirement: System places compiled binary in build directory
The system SHALL emit the compiled binary into a `build/` subdirectory instead of the project root.

#### Scenario: Binary location on Windows
- **WHEN** `fun build` completes successfully on Windows
- **THEN** the compiled binary exists at `build/app-windows-amd64.exe` and NOT at `app.exe` in the project root

#### Scenario: Binary location on Linux
- **WHEN** `fun build` completes successfully on Linux
- **THEN** the compiled binary exists at `build/app-linux-amd64` and NOT at `app` in the project root

#### Scenario: Build directory created automatically
- **WHEN** `fun build` runs and the `build/` directory does not exist
- **THEN** the `build/` directory is created before compilation
