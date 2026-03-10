## ADDED Requirements

### Requirement: System builds release binaries
The fun build system SHALL compile optimized binaries for release.

#### Scenario: Build Windows release
- **WHEN** release is prepared
- **THEN** `fun-windows-amd64.exe` is compiled with `-O3 -DNDEBUG`

#### Scenario: Build Linux release
- **WHEN** release is prepared
- **THEN** `fun-linux-amd64` is compiled with `-O3 -DNDEBUG`

#### Scenario: Build macOS release
- **WHEN** release is prepared
- **THEN** `fun-darwin-amd64` is compiled with `-O3 -DNDEBUG`
