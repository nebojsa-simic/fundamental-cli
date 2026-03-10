## ADDED Requirements

### Requirement: User can scaffold new test module
The system SHALL create a new test module with `fun test add <module>`.

#### Scenario: Create test directory
- **WHEN** user runs `fun test add string`
- **THEN** `tests/string/` directory is created

#### Scenario: Generate test file
- **WHEN** test is scaffolded
- **THEN** `tests/string/test.c` is created with template

#### Scenario: Generate build scripts
- **WHEN** test is scaffolded
- **THEN** `build-windows-amd64.bat` and `build-linux-amd64.sh` are created
