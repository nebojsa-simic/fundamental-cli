## Why

Fundamental library projects need a simple, distributed dependency management system similar to Go modules. Currently, there's no standard way to declare, install, or manage dependencies - each project manually vendors copies without tracking versions or sources.

## What Changes

- **New**: `fun add <dependency>` to add dependencies with automatic installation
- **New**: Support for multiple dependency formats:
  - `fundamental` shorthand (special-cased to canonical repo)
  - `user/repo` GitHub shorthand (HTTPS default)
  - Full HTTPS URLs: `https://github.com/user/repo.git`
  - Full SSH URLs: `git@github.com:user/repo.git`
  - Local paths: `../local-lib` or `file:../local-lib`
- **New**: `fun install` to install all dependencies from `fun.ini`
- **New**: `fun remove <dependency>` to remove dependencies
- **New**: `fun list` to show installed and declared dependencies
- **New**: `fun.ini` `[dependencies]` section support
- **New**: Git clone with version/tag checkout support
- **New**: Local path copying for file-based dependencies
- **New**: Vendor directory management (always copy, never symlink)

## Capabilities

### New Capabilities

- `dependency-parsing`: Parse dependency specifiers in multiple formats (shorthand, HTTPS, SSH, local)
- `git-resolution`: Clone git repositories and checkout specific versions/tags
- `local-resolution`: Copy dependencies from local filesystem paths
- `vendor-management`: Manage `vendor/` directory with copy-based installation
- `ini-dependencies`: Read and write `[dependencies]` section in `fun.ini`
- `version-checkout`: Git tag/branch checkout for versioned dependencies
- `dependency-removal`: Clean removal from `fun.ini` and `vendor/`

### Modified Capabilities

<!-- No existing capabilities modified -->

## Impact

- **Code**: New modules in `src/package/resolver.c/h`, `src/package/git.c/h`, `src/package/local.c/h`, `src/semver/parse.c/h`
- **Commands**: `src/commands/cmd_add.c/h`, `src/commands/cmd_install.c/h`, `src/commands/cmd_remove.c/h`, `src/commands/cmd_list.c/h`
- **fun.ini**: New `[dependencies]` section with key=value format
- **Git**: Git CLI integration for clone, fetch, checkout operations
- **Filesystem**: Vendor directory creation, copying, removal
