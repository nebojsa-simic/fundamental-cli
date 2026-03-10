## ADDED Requirements

### Requirement: User can initialize new fundamental project
The system SHALL generate a complete fundamental library project structure in the current directory.

#### Scenario: Initialize project in current directory
- **WHEN** user runs `fun init` in empty directory
- **THEN** system creates project structure in current directory

#### Scenario: Reject existing files
- **WHEN** user runs `fun init` in non-empty directory
- **THEN** system shows error and exits without modifying files

#### Scenario: Reject if not in project directory
- **WHEN** user runs `fun init` without creating project folder first
- **THEN** system initializes in current working directory

### Requirement: Project structure follows fundamental conventions
The generated project SHALL include standard directories: `src/`, `commands/`, `vendor/`, `.opencode/skills/`.

#### Scenario: Create source directory
- **WHEN** project is scaffolded
- **THEN** `src/` directory contains `main.c`, `cli.c`, `cli.h`

#### Scenario: Create commands directory
- **WHEN** project is scaffolded
- **THEN** `commands/` directory contains `cmd_version.c`, `cmd_version.h`

#### Scenario: Create vendor directory
- **WHEN** project is scaffolded
- **THEN** `vendor/fundamental/` contains copied fundamental library

### Requirement: Generated files use embedded templates
All scaffolded files SHALL be generated from C string templates embedded in the fun binary.

#### Scenario: Generate main.c from template
- **WHEN** project is scaffolded
- **THEN** `src/main.c` is created from embedded template with proper includes

#### Scenario: Generate build scripts from template
- **WHEN** project is scaffolded
- **THEN** `build-windows-amd64.bat` and `build-linux-amd64.sh` are created from templates
