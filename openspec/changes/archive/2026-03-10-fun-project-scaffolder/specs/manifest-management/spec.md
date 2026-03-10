## ADDED Requirements

### Requirement: System generates fun.ini manifest
The system SHALL create a `fun.ini` file with project metadata and configuration.

#### Scenario: Generate fun.ini with project info
- **WHEN** project is scaffolded
- **THEN** `fun.ini` contains `name`, `version`, `description`, `entry`, `output`

#### Scenario: Set default version
- **WHEN** fun.ini is generated
- **THEN** `version` is set to `0.1.0` by default

### Requirement: fun.ini uses flat INI format
The manifest SHALL use simple key=value pairs with optional sections for organization.

#### Scenario: Parse flat properties
- **WHEN** fun.ini is read
- **THEN** top-level keys are parsed as project metadata

#### Scenario: Parse sections
- **WHEN** fun.ini contains `[dependencies]` or `[build]` sections
- **THEN** section properties are parsed into structured config
