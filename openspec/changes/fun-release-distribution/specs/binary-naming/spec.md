## ADDED Requirements

### Requirement: Binaries follow naming convention
Release binaries SHALL use platform-specific naming for clarity.

#### Scenario: Name Windows binary
- **WHEN** Windows binary is built
- **THEN** it's named `fun-windows-amd64.exe`

#### Scenario: Name Linux binary
- **WHEN** Linux binary is built
- **THEN** it's named `fun-linux-amd64`

#### Scenario: Name macOS binary
- **WHEN** macOS binary is built
- **THEN** it's named `fun-darwin-amd64`
