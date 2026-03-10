## ADDED Requirements

### Requirement: System manages vendor directory
The system SHALL maintain `vendor/` directory with copied dependencies.

#### Scenario: Create vendor directory
- **WHEN** first dependency is added
- **THEN** `vendor/` directory is created if it doesn't exist

#### Scenario: Copy dependency
- **WHEN** dependency is installed
- **THEN** it's copied to `vendor/<dependency-name>/`

#### Scenario: Never symlink
- **WHEN** installing any dependency
- **THEN** system always copies, never creates symlinks
