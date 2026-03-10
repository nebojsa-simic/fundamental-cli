## ADDED Requirements

### Requirement: System reports test results
The system SHALL display colored pass/fail output for each test.

#### Scenario: Show passing test
- **WHEN** test exits with code 0
- **THEN** system shows green ✓ with test name

#### Scenario: Show failing test
- **WHEN** test exits with non-zero code
- **THEN** system shows red ✗ with test name

#### Scenario: Show summary
- **WHEN** all tests complete
- **THEN** system shows "X passed, Y failed" summary

#### Scenario: Exit with failure code
- **WHEN** any test fails
- **THEN** `fun test` exits with code 1
