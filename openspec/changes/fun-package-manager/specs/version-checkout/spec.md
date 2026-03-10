## ADDED Requirements

### Requirement: System checks out versioned dependencies
The system SHALL checkout specific tags or branches for versioned dependencies.

#### Scenario: Checkout semantic version
- **WHEN** adding `user/repo@v1.2.3`
- **THEN** system runs `git checkout v1.2.3`

#### Scenario: Checkout branch
- **WHEN** adding `user/repo@develop`
- **THEN** system runs `git checkout develop`

#### Scenario: Handle missing version
- **WHEN** specified version doesn't exist
- **THEN** system shows error and removes cloned directory
