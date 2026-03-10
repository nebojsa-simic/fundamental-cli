## ADDED Requirements

### Requirement: System copies local dependencies
The system SHALL copy local filesystem dependencies to vendor.

#### Scenario: Copy from relative path
- **WHEN** adding `../local-lib`
- **THEN** system copies directory to `vendor/local-lib`

#### Scenario: Copy with file prefix
- **WHEN** adding `file:../local-lib`
- **THEN** system strips `file:` prefix and copies to `vendor/local-lib`

#### Scenario: Validate local path exists
- **WHEN** local path doesn't exist
- **THEN** system shows error and exits without modifying fun.ini
