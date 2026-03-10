## ADDED Requirements

### Requirement: System clones git dependencies
The system SHALL clone git repositories for remote dependencies.

#### Scenario: Clone from GitHub
- **WHEN** adding `user/repo` dependency
- **THEN** system runs `git clone https://github.com/user/repo` to `vendor/`

#### Scenario: Checkout specific version
- **WHEN** adding `user/repo@v1.2.3`
- **THEN** system checks out tag `v1.2.3` after clone

#### Scenario: Remove git metadata
- **WHEN** git clone completes
- **THEN** `.git/` directory is removed to save space
