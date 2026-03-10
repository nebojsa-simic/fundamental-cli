## ADDED Requirements

### Requirement: System copies essential fundamental library folders
The system SHALL copy only essential folders from the fundamental library to `vendor/fundamental/` during initialization.

#### Scenario: Copy arch folder
- **WHEN** fundamental is vendored
- **THEN** `vendor/fundamental/arch/` is copied (platform-specific implementations)

#### Scenario: Copy include folder
- **WHEN** fundamental is vendored
- **THEN** `vendor/fundamental/include/` is copied (public API headers)

#### Scenario: Copy src folder
- **WHEN** fundamental is vendored
- **THEN** `vendor/fundamental/src/` is copied (core implementations)

#### Scenario: Exclude tests folder
- **WHEN** fundamental is vendored
- **THEN** `tests/` folder is NOT copied (reduces vendor size)

#### Scenario: Exclude openspec folder
- **WHEN** fundamental is vendored
- **THEN** `openspec/` folder is NOT copied (not needed for projects)

#### Scenario: Exclude node_modules
- **WHEN** fundamental is vendored
- **THEN** `node_modules/` folder is NOT copied (not needed for C projects)

#### Scenario: Exclude other non-essential folders
- **WHEN** fundamental is vendored
- **THEN** only `arch/`, `include/`, `src/` are copied (everything else excluded)

### Requirement: Copy from local path or git clone
The system SHALL copy from `../fundamental` if available, otherwise clone from git.

#### Scenario: Copy from local path
- **WHEN** `../fundamental` exists and `fun init` is run
- **THEN** system copies only `arch/`, `include/`, `src/` to `vendor/fundamental/`

#### Scenario: Clone from git
- **WHEN** `../fundamental` does not exist
- **THEN** system clones from `https://github.com/nebojsa-simic/fundamental` and copies only essential folders

#### Scenario: Remove git metadata
- **WHEN** fundamental is cloned from git
- **THEN** `.git/` directory is removed after copy to save space

### Requirement: Vendor copy is complete but minimal
The copied fundamental library SHALL include only essential source files and headers.

#### Scenario: Copy source files
- **WHEN** fundamental is vendored
- **THEN** `vendor/fundamental/src/` contains all source files (excluding tests)

#### Scenario: Copy include files
- **WHEN** fundamental is vendored
- **THEN** `vendor/fundamental/include/` contains all header files

#### Scenario: Copy arch files
- **WHEN** fundamental is vendored
- **THEN** `vendor/fundamental/arch/` contains all platform-specific implementations
