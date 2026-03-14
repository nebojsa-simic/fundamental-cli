# Fundamental CLI - Claude Code Instructions

## Overview

CLI application built with the [Fundamental Library](../fundamental/). Demonstrates the library's command-based architecture pattern. Binary: `fun.exe`.

## Build Commands

- **Windows**: `build-windows-amd64.bat`
- **Linux**: `./build-linux-amd64.sh`
- **Tests**: `test-windows-amd64.bat`
- **Code Formatting**: `code-format.bat`
- **Vendor Update**: `vendor-fundamental.bat` (copies latest fundamental library into `vendor/`)

## Smoke Tests

End-to-end smoke tests that verify the full `fun init` → vendor → `fun build` → run cycle:

- **Windows**: `smoke-test-windows-amd64.bat`
- **Linux**: `./smoke-test-linux-amd64.sh`

**Run the smoke test after any change to:** `fun init` templates, `fun build` logic, executor, detector, generator, or platform detection. The smoke test creates `../fundamental-cli-smoke/`, runs the full workflow, and reports pass/fail counts.

## Architecture

```
src/
  main.c              Entry point (cli_main)
  cli.c / cli.h       CLI runtime: init, register, run
  commands/            One file pair per command (cmd_<name>.c/.h)
vendor/
  fundamental/         Vendored copy of fundamental library
tests/
  asyncFileWrite/      Test suites
openspec/              Change management
```

## Adding Commands

1. Create `src/commands/cmd_<name>.c` and `cmd_<name>.h`
2. Implement: `int cmd_<name>_execute(int argc, const char **argv)`
3. Register in `src/main.c`:
```c
cli_register((Command){
    .name = "<name>",
    .description = "Description",
    .execute = cmd_<name>_execute
});
```

## Current Commands

| Command | Description |
|---------|-------------|
| `version` | Show version information |
| `help` | Show help message |
| `init` | Initialize new fundamental project |
| `build` | Build project for current platform |
| `clean` | Remove build artifacts |

## Vendored Fundamental Skills

The fundamental library's AI agent skills are vendored alongside the source code into `vendor/fundamental/.opencode/skills/`. These provide copy-paste patterns for every fundamental module (file I/O, memory, string, async, filesystem, platform, etc.). When writing code that uses the fundamental library, load the relevant skill for ready-made examples.

| Skill file | Domain |
|------------|--------|
| `fundamental-file-io.md` | Read, write, append, stream I/O |
| `fundamental-memory.md` | Allocate, free, copy, fill, compare |
| `fundamental-console.md` | Output, error messages |
| `fundamental-directory.md` | Create, list, remove directories |
| `fundamental-string.md` | Copy, join, template, convert |
| `fundamental-collections.md` | Arrays, hashmaps, sets, RB-trees |
| `fundamental-async.md` | Await, poll, spawn processes |
| `fundamental-platform.md` | Detect OS/arch |
| `fundamental-skills-index.md` | Central cross-reference |

## Rules

- Uses vendored fundamental library from `vendor/fundamental/` — includes reference it via `vendor/fundamental/include/`
- All fundamental library rules apply: no C stdlib, `fun_` prefix, explicit error handling, caller-allocated memory
- See `../fundamental/CLAUDE.md` for full library conventions

## OpenSpec Workflow

Same workflow as fundamental library. See `openspec/` for specs and changes.
