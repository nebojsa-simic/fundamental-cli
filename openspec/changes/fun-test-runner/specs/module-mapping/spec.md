## ADDED Requirements

### Requirement: System maps modules to source files
The system SHALL know which fundamental source files are needed for each module.

#### Scenario: Map string module
- **WHEN** `fun test add string` is run
- **THEN** build script includes `src/string/*.c` files

#### Scenario: Map memory module
- **WHEN** `fun test add memory` is run
- **THEN** build script includes `arch/memory/*/memory.c`

#### Scenario: Map console module
- **WHEN** `fun test add console` is run
- **THEN** build script includes `src/console/console.c` and arch file
