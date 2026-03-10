## ADDED Requirements

### Requirement: System copies fundamental library
The system SHALL copy the fundamental library to `vendor/fundamental/` during initialization.

#### Scenario: Copy from local path
- **WHEN** `../fundamental` exists and `fun init` is run
- **THEN** system copies entire fundamental repo to `vendor/fundamental/`

#### Scenario: Clone from git
- **WHEN** `../fundamental` does not exist
- **THEN** system clones from `https://github.com/nebojsa-simic/fundamental` to `vendor/fundamental/`

#### Scenario: Remove git metadata
- **WHEN** fundamental is cloned from git
- **THEN** `.git/` directory is removed after copy to save space

### Requirement: Vendor copy is complete
The copied fundamental library SHALL include all source files, headers, and skills.

#### Scenario: Copy source files
- **WHEN** fundamental is vendored
- **THEN** `vendor/fundamental/src/` contains all source files

#### Scenario: Copy include files
- **WHEN** fundamental is vendored
- **THEN** `vendor/fundamental/include/` contains all header files

#### Scenario: Copy arch files
- **WHEN** fundamental is vendored
- **THEN** `vendor/fundamental/arch/` contains platform-specific implementations
