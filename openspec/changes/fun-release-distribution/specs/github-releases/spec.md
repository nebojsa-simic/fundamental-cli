## ADDED Requirements

### Requirement: System publishes to GitHub Releases
The release process SHALL upload binaries to GitHub Releases.

#### Scenario: Create release
- **WHEN** release is published
- **THEN** GitHub release is created with version tag

#### Scenario: Upload binaries
- **WHEN** release is published
- **THEN** all platform binaries are uploaded as assets

#### Scenario: Generate release notes
- **WHEN** release is published
- **THEN** release notes are generated from git commits
