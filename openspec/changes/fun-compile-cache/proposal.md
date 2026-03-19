## Why

The `fun compile` pipeline (from the fun-c-compiler change) has 16 stages, each producing a serializable intermediate result. Without caching, every compilation re-runs the full pipeline even when source files haven't changed. For a project like fundamental-cli with 10+ source files each including dozens of headers, this means redundant work on every build. A per-file cache with per-stage invalidation gives fast incremental builds and fast iteration when developing the compiler itself.

## What Changes

- **New**: `.fun/cache/` directory for per-file compilation cache
- **New**: Cache manifest (`manifest.bin`) tracking per-file entries with mtime, content hash, dependency hashes, and per-stage compiler hashes
- **New**: Two-level invalidation: mtime check (fast) then content hash (correct)
- **New**: Per-stage compiler invalidation — changing `parser.c` only invalidates from `--ast-parsed` forward, not the full pipeline
- **New**: Shared code invalidation — changes to foundational files (`type.c`, `symtab.c`, `token.c`, `serialize.c`, `linemap.c`) invalidate all stages
- **New**: Pipeline resume — when a cache miss occurs partway through, compilation resumes from the earliest invalid stage using cached results for prior stages

## Capabilities

### New Capabilities

- `compile-cache`: Per-file compilation cache with two-level invalidation (mtime + content hash), per-stage compiler code tracking, and pipeline resume from earliest invalid stage

### Modified Capabilities

## Impact

- **Build system**: `fun compile` and `fun build --compiler=funcc` gain automatic caching with no user flags required
- **Disk usage**: `.fun/cache/` stores intermediate results per source file — can grow large for big projects; `fun clean` should clear it
- **Correctness**: Cache must never serve stale results — two-level check (mtime then hash) prevents this
- **Dependency on fun-c-compiler**: This change builds on top of the fun-c-compiler pipeline; the cache wraps around the existing stage architecture
