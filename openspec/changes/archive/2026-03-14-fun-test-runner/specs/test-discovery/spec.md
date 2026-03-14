## ADDED Requirements

### Requirement: System discovers test modules
The system SHALL scan `tests/` directory to find all test modules.

#### Scenario: Find test directories
- **WHEN** `fun test` is run
- **THEN** all `tests/*/` subdirectories are discovered

#### Scenario: Handle missing tests
- **WHEN** `tests/` directory doesn't exist
- **THEN** system shows message "No tests found" and exits successfully

#### Scenario: Skip non-test directories
- **WHEN** `tests/` contains non-test files
- **THEN** only directories with `test.c` are treated as test modules
