## Why

Developers need a quick way to bootstrap new fundamental library projects with proper structure, build scripts, and best practices. Currently, each new project requires manual setup of directory structure, build scripts, vendor management, and configuration files.

## What Changes

- **New**: `fun init` command to scaffold new fundamental-based projects in current directory
- **New**: Embedded project templates as C strings in the fun binary
- **New**: Automatic fundamental library copying from local `../fundamental` or git clone
- **New**: `fun.ini` manifest file for project configuration and dependencies
- **New**: `.opencode/skills/fundamental-expert/SKILL.md` scaffolding for LLM assistance
- **New**: Minimal `arch/startup/<platform>/` scaffolding following fundamental principles
- **New**: Platform-specific build script generation (`build-windows-amd64.bat`, `build-linux-amd64.sh`)
- **Modified**: `fun init` does not accept project name - always initializes current directory
- **Modified**: Only copy essential fundamental folders (`arch/`, `include/`, `src/`) to reduce size

## Capabilities

### New Capabilities

- `project-scaffolding`: Complete project structure generation with templates for main.c, cli.c, cli.h, commands, and build scripts
- `manifest-management`: INI-based fun.ini parsing and generation for project metadata and dependencies
- `vendor-copy`: Automated copying of essential fundamental library folders (`arch/`, `include/`, `src/`)
- `arch-scaffolding`: Minimal architecture folder structure following fundamental library patterns
- `skill-copy`: Automatic scaffolding of fundamental-expert skill for LLM assistance

### Modified Capabilities

<!-- No existing capabilities modified -->

## Impact

- **Code**: New command handlers in `src/commands/cmd_init.c/h`
- **Dependencies**: Requires fundamental's config module for INI parsing
- **Templates**: Embedded C strings for project templates (main.c, cli.c, build scripts, fun.ini)
- **Filesystem**: Creates directory structure, copies specific folders, manages vendor/ folder
- **Platform**: Cross-platform path handling for Windows and Linux/macOS
- **Workflow**: Users create directory first, cd into it, then run `fun init`
- **Size**: Reduced vendor size by excluding tests, openspec, node_modules, and other non-essential folders
