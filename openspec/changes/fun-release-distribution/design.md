## Context

Users need a simple way to install fun CLI without building from source. Currently there's no release process, no pre-built binaries, and no installation instructions. A curl-based installation provides the lowest-friction onboarding.

## Goals / Non-Goals

**Goals:**
- GitHub Releases workflow for publishing binaries
- Platform-specific binary naming (fun-<platform>-<arch>)
- README with curl installation commands
- Release build script to compile and upload
- Automated release notes from git commits

**Non-Goals:**
- Package manager installs (brew, chocolatey) for MVP
- Binary signing/verification (can add later)
- Self-update mechanism (can add later)

## Decisions

### Decision 1: GitHub Releases for distribution
**Rationale:** Free, integrated with git, familiar to developers, supports large binaries.

**Alternatives considered:**
- Custom CDN: Infrastructure cost, complexity
- npm/pip: Wrong ecosystem for C binary

### Decision 2: curl-based installation
**Rationale:** One-line install, no dependencies, follows rustup/gh cli pattern.

**Alternatives considered:**
- Install script: Extra download, complexity
- Manual download: More steps for users

### Decision 3: Platform-specific naming
**Rationale:** Clear which binary is which, prevents confusion.

**Alternatives considered:**
- Generic names (fun.exe): Can't host multiple platforms
- Version in filename: Redundant with release tag

### Decision 4: Automated release notes
**Rationale:** Saves time, consistent format, captures all changes.

**Alternatives considered:**
- Manual release notes: Time-consuming, error-prone
- No release notes: Poor user experience

## Risks / Trade-offs

**[GitHub release size limits]** → Mitigation: Binaries are small (<1MB each), well under limits

**[curl not available everywhere]** → Mitigation: Provide wget alternative, manual download option

**[No automatic updates]** → Mitigation: Document manual update process, can add self-update later

## Migration Plan

1. Build fun for all platforms
2. Test on clean systems (VM or container)
3. Create GitHub release with tag (v0.1.0)
4. Upload binaries as assets
5. Update README with installation commands
6. Announce release

## Open Questions

- Should we support nightly/dev builds?
- Should releases be automated via CI or manual?
