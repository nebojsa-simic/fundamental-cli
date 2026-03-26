## ADDED Requirements

### Requirement: Overlay files use TOML subset format
The system SHALL define overlay files as TOML documents supporting a strict subset: string values, string arrays, and named sections only.

#### Scenario: Parse sources array
- **WHEN** overlay contains `sources = ["a.c", "b.c"]`
- **THEN** system includes those files relative to the library root in vendor/

#### Scenario: Parse includes array
- **WHEN** overlay contains `includes = ["include"]`
- **THEN** system adds those paths as `-I vendor/<lib>/<path>` in the compiler invocation

#### Scenario: Parse platform section
- **WHEN** overlay contains `[build.windows-amd64]` with `sources_extra`
- **THEN** system appends those files to the sources list when building on windows-amd64

#### Scenario: Parse linux platform section
- **WHEN** overlay contains `[build.linux-amd64]` with `sources_extra`
- **THEN** system appends those files to the sources list when building on linux-amd64

### Requirement: Overlay files live in vendor/.fun/
The system SHALL read and write overlay files at `vendor/.fun/<libname>.toml`, where `<libname>` matches the directory name of the library under `vendor/`.

#### Scenario: Locate overlay for known library
- **WHEN** resolving vendor files for a library at `vendor/libpng/`
- **THEN** system looks for overlay at `vendor/.fun/libpng.toml`

#### Scenario: Missing overlay does not error
- **WHEN** no overlay exists at `vendor/.fun/<libname>.toml`
- **THEN** system falls through to the next resolution tier without error

### Requirement: Overlay declares sub-dependencies
The system SHALL support a `[dependencies]` section listing other vendor libraries that must also be resolved.

#### Scenario: Resolve sub-dependency
- **WHEN** overlay contains `[dependencies]` with `zlib = "vendor/zlib"`
- **THEN** system also resolves `vendor/zlib` using its own overlay or fallback

#### Scenario: Circular dependency detection
- **WHEN** two overlays declare each other as dependencies
- **THEN** system detects the cycle and reports an error rather than looping

### Requirement: Generated overlays are marked
The system SHALL write `generated = true` in the `[library]` section of any overlay it auto-generates, so hand-authored overlays can be distinguished.

#### Scenario: Generated overlay is identifiable
- **WHEN** `fun build` writes an overlay after auto-generation
- **THEN** the file contains `generated = true` under `[library]`

#### Scenario: Hand-authored overlay is not overwritten
- **WHEN** an overlay exists without `generated = true`
- **THEN** `fun build` does not overwrite it during auto-generation
