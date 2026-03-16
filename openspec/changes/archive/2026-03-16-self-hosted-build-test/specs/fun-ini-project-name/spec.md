## ADDED Requirements

### Requirement: Generator reads project name from fun.ini
The system SHALL read the `name` field from `fun.ini` to derive the output binary name.

#### Scenario: fun.ini present with name field
- **WHEN** `fun build` runs and `fun.ini` exists with `name = <value>`
- **THEN** the generated script outputs the binary as `build/<value>-windows-amd64.exe` (Windows) or `build/<value>-linux-amd64` (Linux)

#### Scenario: fun.ini absent
- **WHEN** `fun build` runs and `fun.ini` does not exist
- **THEN** the generator falls back to `app` as the binary base name

#### Scenario: fun.ini present without name field
- **WHEN** `fun build` runs and `fun.ini` exists but has no `name =` line
- **THEN** the generator falls back to `app` as the binary base name
