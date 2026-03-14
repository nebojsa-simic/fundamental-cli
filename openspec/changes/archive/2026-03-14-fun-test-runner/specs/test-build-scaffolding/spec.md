## ADDED Requirements

### Requirement: System scaffolds test build scripts
The system SHALL generate build scripts for test modules that lack them.

#### Scenario: Detect missing build script
- **WHEN** test module has no `build-windows-amd64.bat`
- **THEN** system generates build script before running test

#### Scenario: Generate Windows test build
- **WHEN** generating test build on Windows
- **THEN** `build-windows-amd64.bat` is created in test directory

#### Scenario: Generate Linux test build
- **WHEN** generating test build on Linux
- **THEN** `build-linux-amd64.sh` is created in test directory
