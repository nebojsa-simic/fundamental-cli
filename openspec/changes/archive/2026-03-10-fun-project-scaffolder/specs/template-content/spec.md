## ADDED Requirements

### Requirement: System embeds main.c template
The system SHALL include an embedded C string template for src/main.c.

#### Scenario: Generate main.c
- **WHEN** project is scaffolded
- **THEN** src/main.c is created with proper includes and cli_main function

**Template Content:**
```c
#include "commands/cmd_version.h"
#include "commands/cmd_help.h"
#include "cli.h"

int main(int argc, const char **argv) {
  cli_init();
  cli_register((Command){.name = "version",
                         .description = "Show version information",
                         .execute = cmd_version_execute});
  cli_register((Command){.name = "help",
                         .description = "Show this help message",
                         .execute = cmd_help_execute});
  return cli_run(argc, argv);
}
```

### Requirement: System embeds cli.h template
The system SHALL include an embedded C string template for src/cli.h with Command struct and function declarations.

#### Scenario: Generate cli.h
- **WHEN** project is scaffolded
- **THEN** src/cli.h is created with Command struct, cli_init, cli_register, cli_run, cli_show_help declarations

### Requirement: System embeds cli.c template
The system SHALL include an embedded C string template for src/cli.c with CLI implementation.

#### Scenario: Generate cli.c
- **WHEN** project is scaffolded
- **THEN** src/cli.c is created with MAX_COMMANDS=16, command registration, help display, command routing

### Requirement: System embeds command templates
The system SHALL include embedded C string templates for command files.

#### Scenario: Generate cmd_version.h and cmd_version.c
- **WHEN** project is scaffolded
- **THEN** commands/cmd_version.h/c created with version output "fun v0.1.0"

#### Scenario: Generate cmd_help.h and cmd_help.c
- **WHEN** project is scaffolded
- **THEN** commands/cmd_help.h/c created that calls cli_show_help()

### Requirement: System embeds build script templates
The system SHALL include embedded C string templates for build scripts.

#### Scenario: Generate build-windows-amd64.bat
- **WHEN** project is scaffolded
- **THEN** Windows batch file created with GCC flags: -nostdlib -fno-builtin -e main -mconsole

#### Scenario: Generate build-linux-amd64.sh
- **WHEN** project is scaffolded
- **THEN** Linux shell script created with GCC flags: -nostdlib -fno-builtin -e main

### Requirement: System embeds fun.ini template
The system SHALL include an embedded C string template for fun.ini.

#### Scenario: Generate fun.ini
- **WHEN** project is scaffolded
- **THEN** fun.ini created with name, version=0.1.0, entry=src/main.c, output=fun.exe, [dependencies] fundamental=local

### Requirement: System embeds README.md template
The system SHALL include an embedded C string template for README.md.

#### Scenario: Generate README.md
- **WHEN** project is scaffolded
- **THEN** README.md created with Features, Building, Usage, Adding Commands sections

### Requirement: System embeds arch startup templates
The system SHALL include embedded C string templates for arch/startup files.

#### Scenario: Generate arch/startup/windows-amd64/windows.c
- **WHEN** project is scaffolded
- **THEN** Windows startup file created with __main stub and windows.h include

#### Scenario: Generate arch/startup/linux-amd64/linux.c
- **WHEN** project is scaffolded
- **THEN** Linux startup file created with __main stub
