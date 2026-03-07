# fundamental-cli

A command-line interface built with the [fundamental](https://github.com/nebojsa-simic/fundamental) library.

## Features

- **Zero stdlib runtime dependencies** - Uses only fundamental library
- **Line-buffered console output** - Efficient output with 512-byte buffer
- **Cross-platform** - Works on Windows and POSIX systems
- **Command-based architecture** - Easy to extend with new commands

## Building

### Windows

```batch
.\build-windows-amd64.bat
```

### Linux

```bash
./build-linux-amd64.sh
```

## Usage

```bash
# Show help
./cli --help
./cli help

# Show version
./cli version
```

## Adding Commands

1. Create command files in `commands/` directory:
   - `cmd_yourcommand.h`
   - `cmd_yourcommand.c`

2. Implement the execute function:
```c
int cmd_yourcommand_execute(int argc, const char **argv) {
    fun_console_write_line("Your command output");
    return 0;
}
```

3. Register the command in `src/main.c`:
```c
cli_register((Command){
    .name = "yourcommand",
    .description = "Description of your command",
    .execute = cmd_yourcommand_execute
});
```

## Architecture

```
fundamental-cli/
├── src/
│   ├── main.c          # Entry point
│   ├── cli.c           # CLI runtime
│   └── cli.h           # CLI interface
├── commands/
│   ├── cmd_version.c   # Version command
│   └── cmd_help.c      # Help command
├── vendor/
│   └── fundamental/    # Vendored fundamental library
└── build-*.sh/bat      # Build scripts
```

## License

Same license as fundamental library.
