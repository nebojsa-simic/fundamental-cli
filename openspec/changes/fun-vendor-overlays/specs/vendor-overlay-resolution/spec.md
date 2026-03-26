## ADDED Requirements

### Requirement: Build resolves vendor files via three-tier fallback
The system SHALL resolve the list of vendor `.c` files for each library in `vendor/` using a strict priority order: overlay → include-scan → symbol-resolution fallback.

#### Scenario: Tier 1 — overlay present
- **WHEN** `vendor/.fun/<libname>.toml` exists
- **THEN** system uses its `sources` and platform `sources_extra` exclusively, skipping all scanning

#### Scenario: Tier 2 — no overlay, fundamental-style library
- **WHEN** no overlay exists and the library follows `vendor/<lib>/include/<lib>/` structure
- **THEN** system performs include-directive scanning and writes a generated overlay

#### Scenario: Tier 3 — no overlay, unknown library, compiler unavailable
- **WHEN** no overlay exists, library does not follow fundamental conventions, and compiler Phase 2 is not available
- **THEN** system emits a warning naming the library and skips it; build continues without that library's files

#### Scenario: Tier 3 — no overlay, unknown library, compiler available
- **WHEN** no overlay exists, library does not follow fundamental conventions, and compiler Phase 2 is available
- **THEN** system performs symbol-resolution scan and writes a generated overlay

### Requirement: Startup files are always injected
The system SHALL unconditionally include `vendor/fundamental/src/startup/startup.c` and `vendor/fundamental/arch/startup/<platform>/<entry>.c` regardless of overlay content, as these files are never referenced by `#include` directives.

#### Scenario: Startup included without overlay entry
- **WHEN** generating the vendor file list for any project using fundamental
- **THEN** startup files appear first in the list before any overlay-resolved files

### Requirement: Resolved overlay is cached to disk
The system SHALL write a generated overlay to `vendor/.fun/<libname>.toml` after Tier 2 or Tier 3 resolution so subsequent builds skip scanning.

#### Scenario: Cache written after Tier 2 scan
- **WHEN** Tier 2 include-scan completes successfully
- **THEN** `vendor/.fun/<libname>.toml` is written with `generated = true`

#### Scenario: Stale cache is regenerated
- **WHEN** `vendor/.fun/<libname>.toml` exists with `generated = true` and the library directory mtime is newer than the overlay mtime
- **THEN** system discards the cached overlay and re-runs the scan

#### Scenario: Hand-authored overlay is never regenerated
- **WHEN** `vendor/.fun/<libname>.toml` exists without `generated = true`
- **THEN** system uses it as Tier 1 regardless of mtime
