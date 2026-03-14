## ADDED Requirements

### Requirement: User can filter tests by pattern
The system SHALL support glob pattern filtering with `--filter`.

#### Scenario: Filter by exact name
- **WHEN** `fun test --filter string` is run
- **THEN** only tests matching "string" are executed

#### Scenario: Filter with wildcard
- **WHEN** `fun test --filter "string*"` is run
- **THEN** tests starting with "string" are executed

#### Scenario: No matches
- **WHEN** `fun test --filter` matches no tests
- **THEN** system shows "No tests match pattern" and exits 0
