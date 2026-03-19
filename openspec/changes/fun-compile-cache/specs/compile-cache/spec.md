## ADDED Requirements

### Requirement: Per-file compilation caching
The system SHALL cache the output of each pipeline stage per source file in `.fun/cache/`. When a source file and all its dependencies are unchanged, the system SHALL reuse cached results instead of re-running the pipeline.

#### Scenario: Full cache hit on unchanged file
- **WHEN** `fun compile src/main.c` is invoked AND `src/main.c` and all its included headers have not changed since the last compilation
- **THEN** the system SHALL use cached stage outputs and produce the final result without re-running any pipeline stage

#### Scenario: Cache miss on modified source file
- **WHEN** `fun compile src/main.c` is invoked AND `src/main.c` has been modified since the last compilation
- **THEN** the system SHALL re-run the full pipeline from the first stage and update all cached stage outputs

#### Scenario: Cache miss on modified header
- **WHEN** `fun compile src/main.c` is invoked AND a header included by `src/main.c` has been modified since the last compilation
- **THEN** the system SHALL re-run the full pipeline from the first stage and update all cached stage outputs

#### Scenario: First compilation with no cache
- **WHEN** `fun compile src/main.c` is invoked AND no `.fun/cache/` directory exists
- **THEN** the system SHALL create `.fun/cache/`, run the full pipeline, and cache all stage outputs

### Requirement: Two-level invalidation
The system SHALL use a two-level invalidation strategy: file modification time (mtime) as a fast check, then content hash as a correctness check.

#### Scenario: File touched but not modified
- **WHEN** a source file's mtime has changed but its content hash is identical to the cached hash
- **THEN** the system SHALL update the stored mtime in the manifest and treat the file as a cache hit

#### Scenario: Mtime unchanged
- **WHEN** a source file's mtime matches the stored mtime in the manifest
- **THEN** the system SHALL skip the content hash check and treat the file as a cache hit (for that level)

### Requirement: Per-stage compiler invalidation
The system SHALL track the content hash of the compiler source files that implement each pipeline stage. When a stage's compiler code changes, the system SHALL invalidate that stage and all subsequent stages, while preserving cached results for earlier stages.

#### Scenario: Compiler stage code changed
- **WHEN** `fun compile src/main.c` is invoked AND `parser.c` has changed since the last compilation AND `src/main.c` has not changed
- **THEN** the system SHALL reuse cached results for all stages before `ast-parsed` and re-run from `ast-parsed` through `assembly`

#### Scenario: Compiler stage code unchanged
- **WHEN** `fun compile src/main.c` is invoked AND no compiler source files have changed AND `src/main.c` has not changed
- **THEN** the system SHALL reuse all cached stage results

### Requirement: Shared code invalidation
The system SHALL track a combined hash of shared foundational files (`type.c`, `symtab.c`, `token.c`, `serialize.c`, `linemap.c`). When any shared file changes, the system SHALL invalidate all cached stages.

#### Scenario: Shared file changed
- **WHEN** `fun compile src/main.c` is invoked AND `type.c` has changed since the last compilation
- **THEN** the system SHALL re-run the full pipeline for all source files regardless of source file changes

### Requirement: Pipeline resume from earliest invalid stage
When a cache is partially valid (early stages cached, later stages invalidated), the system SHALL resume compilation from the earliest invalid stage using cached results for prior stages.

#### Scenario: Resume from mid-pipeline
- **WHEN** a source file is unchanged but the compiler's `irgen.c` has changed
- **THEN** the system SHALL load the cached `ast-typed` result and resume the pipeline from the `ir` stage forward

### Requirement: Cache storage location
The system SHALL store all cache files in `.fun/cache/` relative to the project root directory.

#### Scenario: Cache directory creation
- **WHEN** `fun compile` is invoked AND `.fun/cache/` does not exist
- **THEN** the system SHALL create the `.fun/cache/` directory before writing any cache files

#### Scenario: Cache directory already exists
- **WHEN** `fun compile` is invoked AND `.fun/cache/` already exists
- **THEN** the system SHALL use the existing directory without modification

### Requirement: Cache clearing via fun clean
The `fun clean` command SHALL remove the `.fun/cache/` directory and all its contents.

#### Scenario: Clean with existing cache
- **WHEN** `fun clean` is invoked AND `.fun/cache/` exists
- **THEN** the system SHALL delete `.fun/cache/` and all files within it

#### Scenario: Clean with no cache
- **WHEN** `fun clean` is invoked AND `.fun/cache/` does not exist
- **THEN** the system SHALL complete without error

### Requirement: Manifest resilience
The system SHALL handle manifest corruption gracefully by rebuilding the cache from scratch.

#### Scenario: Corrupt manifest
- **WHEN** `fun compile` is invoked AND `manifest.bin` exists but fails to parse
- **THEN** the system SHALL delete `manifest.bin`, treat all files as cache misses, and rebuild the manifest from the current compilation results
