# Fundamental CLI - Claude Code Instructions

## Overview

CLI application built with the [Fundamental Library](../fundamental/). Demonstrates the library's command-based architecture pattern. Binary: `fun.exe`.

## Build Commands

- **Windows**: `build-windows-amd64.bat`
- **Linux**: `./build-linux-amd64.sh`
- **Tests**: `test-windows-amd64.bat`
- **Code Formatting**: `code-format.bat`
- **Vendor Update**: `vendor-fundamental.bat` (copies latest fundamental library into `vendor/`)

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

## Rules

- Uses vendored fundamental library from `vendor/fundamental/` — includes reference it via `vendor/fundamental/include/`
- All fundamental library rules apply: no C stdlib, `fun_` prefix, explicit error handling, caller-allocated memory
- See `../fundamental/CLAUDE.md` for full library conventions

## OpenSpec Workflow

Same workflow as fundamental library. See `openspec/` for specs and changes.
