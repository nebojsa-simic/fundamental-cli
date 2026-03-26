## Why

`fun build` currently hardcodes vendor file lists, which breaks silently whenever a new module dependency is added or a library is refactored — and cannot handle third-party libraries dropped into `vendor/` at all. Projects need a way to describe how to build any vendored library without modifying the upstream source.

## What Changes

- **New**: `vendor/.fun/<libname>.toml` overlay format — thin build descriptor for a vendored library (sources, includes, platform variants, sub-dependencies)
- **New**: Overlay registry — a separate registry (distinct from the package registry) hosting community-maintained overlays for popular libraries, in the style of DefinitelyTyped for npm
- **New**: Three-tier vendor resolution in `fun build`: (1) read overlay if present, (2) auto-generate overlay via include scanning for fundamental-style libraries using the tokenizer, (3) symbol-resolution fallback once `fun-c-compiler` reaches Phase 2
- **New**: Auto-generation of `vendor/.fun/fundamental.toml` on first build — scans `#include "fundamental/..."` directives transitively, writes overlay, caches for subsequent builds
- **Modified**: `fun add` (from `fun-package-manager`) — after vendoring a library, silently attempt to fetch its overlay from the overlay registry; continue without error if none exists

## Capabilities

### New Capabilities

- `vendor-overlay-format`: The `fun.vendor.toml` schema — sources, includes, platform sections, sub-dependency declarations
- `vendor-overlay-resolution`: Three-tier resolution logic in `fun build` — overlay lookup, include-scan auto-generation, symbol-resolution fallback
- `vendor-overlay-registry`: Overlay registry protocol — URL scheme, fetch, version matching, caching
- `vendor-overlay-autogen`: Auto-generation of overlays for fundamental-style libraries via tokenizer-based include scanning and transitive module closure

### Modified Capabilities

- `build-generation`: Build script generation must now use resolved vendor file lists from the overlay system instead of hardcoded paths
- `source-scanning`: Source scanning extended to cover `vendor/.fun/` overlay lookup and include-directive scanning of vendor headers

## Impact

- **Code**: New modules `src/vendor/overlay.c/h`, `src/vendor/resolver.c/h`, `src/vendor/registry.c/h`, `src/vendor/autogen.c/h`
- **Commands**: `src/commands/cmd_add.c` gains overlay fetch step
- **Build generation**: `arch/build/<platform>/generator.c` replaced hardcoded vendor list with resolver output
- **Dependencies**: Requires tokenizer (done); symbol-resolution fallback requires `fun-c-compiler` Phase 2 (future, gated)
- **vendor/.fun/**: New directory created and managed by `fun build` and `fun add`
- **Registry**: Separate HTTPS endpoint from package registry; read-only fetch only (no auth required for read)
