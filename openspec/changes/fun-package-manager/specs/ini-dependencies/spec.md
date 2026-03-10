## ADDED Requirements

### Requirement: System reads dependencies from fun.ini
The system SHALL parse `[dependencies]` section for `fun install`.

#### Scenario: Parse dependencies section
- **WHEN** `fun install` is run
- **THEN** `[dependencies]` section is parsed for key=value pairs

#### Scenario: Write new dependency
- **WHEN** `fun add` completes
- **THEN** dependency is added to `[dependencies]` section

#### Scenario: Remove dependency
- **WHEN** `fun remove` is run
- **THEN** dependency line is removed from `[dependencies]`
