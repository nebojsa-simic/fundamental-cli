## Context

Projects currently require manual management of platform-specific build scripts. There's no unified way to build, clean, or configure compilation across Windows, Linux, and macOS. The fun CLI needs build commands that work consistently across platforms.

## Goals / Non-Goals

**Goals:**
- Detect platform and run appropriate build script
- Generate build scripts when missing (scans src/**/*.c)
- Support --release, --clean, --verbose flags
- Integrate with fun.ini for configuration
- Clean build artifacts with fun clean

**Non-Goals:**
- Incremental compilation (full rebuild only for now)
- Parallel compilation optimization
- Custom compiler support (GCC only initially)

## Decisions

### Decision 1: Detect and use existing build scripts first
**Rationale:** Respects user's existing build setup, allows customization, backward compatible.

**Alternatives considered:**
- Always generate: Would overwrite user's scripts
- Never generate: Requires manual setup for every project

### Decision 2: Recursive src/**/*.c scan for generation
**Rationale:** Automatic discovery, no configuration needed, matches fundamental's pattern.

**Alternatives considered:**
- Manual source list in fun.ini: More configuration burden
- Only entry point: Won't work for multi-file projects

### Decision 3: Wrap existing scripts, don't replace
**Rationale:** Users can optimize their scripts, fun provides sensible defaults.

**Alternatives considered:**
- Native build system: Much more complex, can do later
- Replace all scripts: Breaks existing workflows

### Decision 4: Use fundamental's config module for fun.ini
**Rationale:** Already available, follows fundamental patterns, no new dependencies.

**Alternatives considered:**
- Custom INI parser: Reinventing the wheel
- JSON/YAML config: Less aligned with fundamental ecosystem

## Risks / Trade-offs

**[Generated scripts may not match user's optimization needs]** → Mitigation: Users can modify generated scripts, fun respects existing ones

**[Platform detection edge cases]** → Mitigation: Start with Windows/Linux, add macOS later

**[Source scanning can miss files]** → Mitigation: Allow manual override in fun.ini [sources] section

## Open Questions

- Should we support clang as alternative to GCC?
- Should build cache object files for incremental builds?
