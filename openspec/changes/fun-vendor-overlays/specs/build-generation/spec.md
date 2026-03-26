## MODIFIED Requirements

### Requirement: Generated scripts include vendor paths
Build scripts SHALL include vendor paths resolved dynamically via the overlay system rather than a hardcoded list.

#### Scenario: Include vendor fundamental via overlay
- **WHEN** build script is generated and `vendor/.fun/fundamental.toml` exists
- **THEN** it includes `-I vendor/fundamental/include` and all sources listed in the overlay

#### Scenario: Include third-party vendor via overlay
- **WHEN** a library exists in `vendor/` with a corresponding `vendor/.fun/<libname>.toml`
- **THEN** build script includes that library's resolved sources and include paths

#### Scenario: Warn on unresolvable vendor library
- **WHEN** a library exists in `vendor/` with no overlay and no fundamental-style conventions and compiler Phase 2 is unavailable
- **THEN** build script generation emits a warning naming the library and omits it from the script

#### Scenario: Link vendor sources
- **WHEN** build script is generated
- **THEN** it includes startup files unconditionally, then all overlay-resolved sources for each vendor library
