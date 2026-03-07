#ifndef CLI_H
#define CLI_H

#include "vendor/fundamental/include/console/console.h"
#include "vendor/fundamental/include/string/string.h"

/**
 * Command structure for CLI subcommands
 */
typedef struct Command {
  String name;
  String description;
  int (*execute)(int argc, const char **argv);
} Command;

/**
 * Initialize the CLI runtime
 * @return ErrorResult indicating success or failure
 */
ErrorResult cli_init(void);

/**
 * Register a command with the CLI
 * @param cmd Command to register
 * @return ErrorResult indicating success or failure
 */
ErrorResult cli_register(Command cmd);

/**
 * Run the CLI with provided arguments
 * @param argc Argument count
 * @param argv Argument vector
 * @return Exit code (0 for success, non-zero for error)
 */
int cli_run(int argc, const char **argv);

/**
 * Display help message showing all registered commands
 * @return Exit code (always 0)
 */
int cli_show_help(void);

#endif // CLI_H
