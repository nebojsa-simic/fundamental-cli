## ADDED Requirements

### Requirement: System detects current platform
The system SHALL automatically detect the operating system and architecture for build targeting.

#### Scenario: Detect Windows
- **WHEN** `fun build` is run on Windows
- **THEN** platform is identified as `windows-amd64`

#### Scenario: Detect Linux
- **WHEN** `fun build` is run on Linux
- **THEN** platform is identified as `linux-amd64`

#### Scenario: Detect macOS
- **WHEN** `fun build` is run on macOS
- **THEN** platform is identified as `darwin-amd64`
