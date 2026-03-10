#include "commands/cmd_help.h"
#include "commands/cmd_version.h"
#include "commands/cmd_init.h"
#include "src/cli.h"

int cli_main(int argc, const char **argv) {
  // Initialize CLI runtime
  cli_init();

  // Register available commands
  cli_register((Command){.name = "version",
                         .description = "Show version information",
                         .execute = cmd_version_execute});

  cli_register((Command){.name = "help",
                         .description = "Show this help message",
                         .execute = cmd_help_execute});

  cli_register((Command){.name = "init",
                         .description = "Initialize new fundamental project",
                         .execute = cmd_init_execute});

  // Run CLI with provided arguments
  return cli_run(argc, argv);
}
