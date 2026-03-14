## ADDED Requirements

### Requirement: System executes build scripts
The system SHALL run the appropriate build script for the current platform.

#### Scenario: Execute Windows script
- **WHEN** `fun build` is run on Windows
- **THEN** `build-windows-amd64.bat` is executed

#### Scenario: Execute Linux script
- **WHEN** `fun build` is run on Linux
- **THEN** `build-linux-amd64.sh` is executed

#### Scenario: Report build errors
- **WHEN** build script fails
- **THEN** system shows error message and exits with non-zero code

### Requirement: Build supports verbose mode
The system SHALL show detailed output when `--verbose` flag is provided.

#### Scenario: Verbose build output
- **WHEN** `fun build --verbose` is run
- **THEN** all compiler commands are echoed before execution
