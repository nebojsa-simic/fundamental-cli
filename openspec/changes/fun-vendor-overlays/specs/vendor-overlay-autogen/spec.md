## ADDED Requirements

### Requirement: Include-scan detects fundamental module dependencies
The system SHALL tokenize all `src/**/*.c` files and extract `#include "fundamental/..."` directives to determine which fundamental modules are referenced.

#### Scenario: Module detected from source include
- **WHEN** any `src/**/*.c` file contains `#include "fundamental/async/async.h"`
- **THEN** the `async` module is added to the required module set

#### Scenario: Multiple modules from multiple files
- **WHEN** source files collectively reference `fundamental/async`, `fundamental/config`, and `fundamental/filesystem`
- **THEN** all three modules are present in the required module set

### Requirement: Include-scan walks vendor headers transitively
The system SHALL also scan headers under `vendor/fundamental/include/fundamental/<module>/` for `#include "fundamental/..."` directives, adding newly discovered modules and repeating until stable.

#### Scenario: Transitive dependency discovered
- **WHEN** `fundamental/config/config.h` contains `#include "fundamental/hashmap/hashmap.h"`
- **THEN** `hashmap` is added to the required module set even if no user source file includes it directly

#### Scenario: Fixed-point termination
- **WHEN** a scan pass discovers no new modules
- **THEN** scanning stops

### Requirement: Module names expand to source file lists
The system SHALL map each required module name to all `.c` files under `vendor/fundamental/src/<module>/` and all `.c` files under `vendor/fundamental/arch/<module>/<platform>/`.

#### Scenario: Single-file module expansion
- **WHEN** module `hashmap` is required and `vendor/fundamental/src/hashmap/hashmap.c` exists
- **THEN** `hashmap.c` is included in the generated overlay sources

#### Scenario: Multi-file module expansion
- **WHEN** module `config` is required and `vendor/fundamental/src/config/` contains `config.c`, `iniParser.c`, `cliParser.c`
- **THEN** all three are included in the generated overlay sources

#### Scenario: Platform arch files expansion
- **WHEN** module `async` is required and building for `windows-amd64`
- **THEN** `vendor/fundamental/arch/async/windows-amd64/async.c` appears in the `[build.windows-amd64]` `sources_extra` of the generated overlay

### Requirement: Auto-generated overlay is written and committed-safe
The system SHALL write the generated overlay to `vendor/.fun/fundamental.toml` with `generated = true`, and the file SHALL be safe to commit to version control so teammates skip re-scanning.

#### Scenario: Overlay written after first build
- **WHEN** no `vendor/.fun/fundamental.toml` exists and `fun build` runs
- **THEN** `vendor/.fun/fundamental.toml` is created with correct sources

#### Scenario: Committed overlay used on subsequent builds
- **WHEN** `vendor/.fun/fundamental.toml` exists and is not stale
- **THEN** `fun build` reads it directly without scanning
