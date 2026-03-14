## ADDED Requirements

### Requirement: System detects existing build scripts
The system SHALL check for presence of platform-specific build scripts before generation.

#### Scenario: Find Windows build script
- **WHEN** `build-windows-amd64.bat` exists
- **THEN** system uses existing script instead of generating

#### Scenario: Find Linux build script
- **WHEN** `build-linux-amd64.sh` exists
- **THEN** system uses existing script instead of generating

#### Scenario: No build scripts found
- **WHEN** neither build script exists
- **THEN** system generates build scripts from templates
