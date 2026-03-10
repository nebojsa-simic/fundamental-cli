## ADDED Requirements

### Requirement: System executes test binaries
The system SHALL build and run each discovered test module.

#### Scenario: Build test
- **WHEN** running a test module
- **THEN** test's build script is executed first

#### Scenario: Run test binary
- **WHEN** test is built successfully
- **THEN** test binary (`test.exe` or `test`) is executed

#### Scenario: Capture exit code
- **WHEN** test completes
- **THEN** exit code is recorded for reporting
