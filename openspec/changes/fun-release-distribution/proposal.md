## Why

Users need a simple way to install the fun CLI without complex build steps. Currently, there's no release process, no pre-built binaries, and no installation instructions. A curl-based installation (like Rust's rustup or GitHub CLI) provides the lowest-friction onboarding.

## What Changes

- **New**: GitHub Releases workflow for publishing fun binaries
- **New**: Platform-specific binary naming (`fun-windows-amd64.exe`, `fun-linux-amd64`, `fun-darwin-amd64`)
- **New**: README installation section with curl commands for each platform
- **New**: Release build script to compile and upload binaries
- **New**: Automated release notes generation from git commits
- **New**: Binary signing/verification (optional, future)
- **Modified**: fun binary now distributed via GitHub Releases instead of source-only

## Capabilities

### New Capabilities

- `release-building`: Cross-platform compilation for Windows, Linux, macOS
- `binary-naming`: Platform-specific naming convention for releases
- `github-releases`: Integration with GitHub Releases API for upload
- `curl-installation`: One-line installation commands for each platform
- `release-notes`: Automated changelog generation from git history

### Modified Capabilities

- `project-scaffolding`: Scaffolded projects reference fun installation in their README

## Impact

- **CI/CD**: GitHub Actions or similar for automated release builds
- **README**: Installation instructions with platform-specific curl commands
- **Distribution**: Binary releases instead of source-only
- **Versioning**: Semantic versioning for releases (v0.1.0, v1.0.0, etc.)
