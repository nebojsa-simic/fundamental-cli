# AGENTS.md - Fundamental CLI Development Guide

## Build Commands

- **Windows**: `build-windows-amd64.bat` — compiles `fun.exe`
- **Linux**: `./build-linux-amd64.sh` — compiles `fun`
- **Code Formatting**: `code-format.bat` (Windows) / `./code-format.sh` (Linux)
- **Vendor Update**: `vendor-fundamental.bat` — copies latest fundamental library into `vendor/fundamental/`

## Smoke Tests

End-to-end tests that verify the full `fun init` → vendor → `fun build` → run cycle:

- **Windows**: `smoke-test-windows-amd64.bat`
- **Linux**: `./smoke-test-linux-amd64.sh`

**Run the smoke test after any change to:** `fun init` templates, `fun build` logic, executor, detector, generator, or platform detection. The smoke test creates `../fundamental-cli-smoke/`, runs the full workflow end-to-end, and reports pass/fail counts.

## Architecture

```
src/
  main.c              Entry point (cli_main)
  cli.c / cli.h       CLI runtime: init, register, run
  commands/           One file pair per command (cmd_<name>.c/.h)
  build/              Build subsystem (detector, generator, executor, config)
  fun/                CLI-local helpers (platform string, working dir)
vendor/
  fundamental/        Vendored copy of fundamental library
```

## Adding Commands

1. Create `src/commands/cmd_<name>.c` and `cmd_<name>.h`
2. Implement: `int cmd_<name>_execute(int argc, const char **argv)`
3. Register in `src/main.c` with `cli_register(...)`
4. Add both source files to `build-windows-amd64.bat` and `build-linux-amd64.sh`

## Vendored Fundamental Skills

The fundamental library's AI agent skills are vendored into `vendor/fundamental/.opencode/skills/`. Load the relevant skill before writing code that uses the library — each skill contains copy-paste patterns with full error handling for its module.

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

- Uses vendored fundamental library from `vendor/fundamental/` — includes via `vendor/fundamental/include/`
- All fundamental library rules apply: no C stdlib, `fun_` prefix, explicit error handling, caller-allocated memory
- See `../fundamental/CLAUDE.md` and `../fundamental/AGENTS.md` for full library conventions

## OpenSpec Workflow

Same workflow as fundamental library. See `openspec/` for specs and changes.
