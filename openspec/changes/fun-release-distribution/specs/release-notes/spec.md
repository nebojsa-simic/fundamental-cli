## ADDED Requirements

### Requirement: Release notes are generated
The release process SHALL generate changelog from git history.

#### Scenario: Generate from commits
- **WHEN** release is created
- **THEN** release notes include commit messages since last release

#### Scenario: Format release notes
- **WHEN** release notes are generated
- **THEN** they follow conventional commits format

#### Scenario: Include breaking changes
- **WHEN** breaking changes exist
- **THEN** they're highlighted in release notes
