## ADDED Requirements

### Requirement: System parses dependency specifiers
The system SHALL parse multiple dependency formats in `fun add`.

#### Scenario: Parse fundamental shorthand
- **WHEN** user runs `fun add fundamental`
- **THEN** dependency is resolved to `https://github.com/nebojsa-simic/fundamental@main`

#### Scenario: Parse GitHub shorthand
- **WHEN** user runs `fun add user/repo`
- **THEN** dependency is resolved to `https://github.com/user/repo@main`

#### Scenario: Parse HTTPS URL
- **WHEN** user runs `fun add https://github.com/user/repo.git`
- **THEN** dependency uses provided HTTPS URL

#### Scenario: Parse SSH URL
- **WHEN** user runs `fun add git@github.com:user/repo.git`
- **THEN** dependency uses provided SSH URL

#### Scenario: Parse local path
- **WHEN** user runs `fun add ../local-lib`
- **THEN** dependency is treated as local filesystem path
