## ADDED Requirements

### Requirement: Users can install via curl
Installation instructions SHALL provide one-line curl commands.

#### Scenario: Install on Windows
- **WHEN** user runs Windows install command
- **THEN** `curl -L https://.../fun-windows-amd64.exe -o fun.exe` downloads binary

#### Scenario: Install on Linux
- **WHEN** user runs Linux install command
- **THEN** `curl -L https://.../fun-linux-amd64 -o fun && chmod +x fun` downloads and makes executable

#### Scenario: Install on macOS
- **WHEN** user runs macOS install command
- **THEN** `curl -L https://.../fun-darwin-amd64 -o fun && chmod +x fun` downloads and makes executable
