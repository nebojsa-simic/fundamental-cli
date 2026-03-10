## Context

There's no standard way to declare, install, or manage dependencies for fundamental projects. Each project manually vendors copies without tracking versions or sources. A distributed dependency management system (like Go modules) is needed.

## Goals / Non-Goals

**Goals:**
- Support multiple dependency formats (shorthand, HTTPS, SSH, local)
- Clone git repos and checkout versions/tags
- Copy local paths to vendor
- Manage vendor/ directory (always copy, never symlink)
- Read/write [dependencies] in fun.ini
- Special-case "fundamental" shorthand to canonical repo

**Non-Goals:**
- Centralized package registry (distributed like Go modules)
- Dependency version resolution (user specifies exact versions)
- Transitive dependency resolution (direct dependencies only for now)

## Decisions

### Decision 1: Distributed model (no central registry)
**Rationale:** Aligns with Go modules, simpler infrastructure, git is the source of truth.

**Alternatives considered:**
- Central registry: More infrastructure, maintenance burden
- Hybrid approach: Complexity without clear benefit

### Decision 2: Support GitHub shorthand user/repo
**Rationale:** Common pattern, shorter than full URLs, HTTPS default works for public repos.

**Alternatives considered:**
- Full URLs only: Verbose for common case
- Custom shorthand syntax: Learning curve for users

### Decision 3: Always copy, never symlink
**Rationale:** Cross-platform compatibility, self-contained projects, no symlink resolution issues.

**Alternatives considered:**
- Symlinks: Faster but Windows compatibility issues
- Hard links: Still has cross-platform complexity

### Decision 4: Remove .git after clone
**Rationale:** Saves disk space, vendor is a snapshot not a working repo.

**Alternatives considered:**
- Keep .git: Users can't easily update anyway
- Shallow clone: Still has .git overhead

### Decision 5: Special-case "fundamental" shorthand
**Rationale:** Most common dependency, deserves shortest syntax.

**Alternatives considered:**
- Require full specifier: Verbose for common case
- Map all common repos: Hard to maintain list

## Risks / Trade-offs

**[No transitive dependencies]** → Mitigation: Document clearly, can add in future version

**[No version resolution]** → Mitigation: Users specify exact versions, semver enforcement later

**[Git required for remote deps]** → Mitigation: Document prerequisite, provide manual download option

## Open Questions

- Should we support version ranges (^1.2.0, ~1.2.0)?
- Should we cache git clones globally or per-project?
