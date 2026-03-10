## ADDED Requirements

### Requirement: User can remove dependencies
The system SHALL remove dependencies from both `fun.ini` and `vendor/`.

#### Scenario: Remove from fun.ini
- **WHEN** user runs `fun remove fundamental`
- **THEN** dependency line is removed from `[dependencies]`

#### Scenario: Remove from vendor
- **WHEN** dependency is removed
- **THEN** `vendor/fundamental/` directory is deleted

#### Scenario: Handle missing dependency
- **WHEN** removing non-existent dependency
- **THEN** system shows warning but doesn't fail
