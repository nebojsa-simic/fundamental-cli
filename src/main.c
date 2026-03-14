#include "src/commands/cmd_help.h"
#include "src/commands/cmd_init.h"
#include "src/commands/cmd_version.h"
#include "src/commands/cmd_build.h"
#include "src/commands/cmd_clean.h"
#include "src/commands/cmd_test.h"
#include "src/commands/cmd_test_add.h"
#include "src/cli.h"

int cli_main(int argc, const char **argv)
{
	// Initialize CLI runtime
	cli_init();

	// Register available commands
	cli_register((Command){ .name = "version",
							.description = "Show version information",
							.execute = cmd_version_execute });

	cli_register((Command){ .name = "help",
							.description = "Show this help message",
							.execute = cmd_help_execute });

	cli_register((Command){ .name = "init",
							.description = "Initialize new fundamental project",
							.execute = cmd_init_execute });

	cli_register((Command){ .name = "build",
							.description = "Build project for current platform",
							.execute = cmd_build_execute });

	cli_register((Command){ .name = "clean",
							.description = "Remove build artifacts",
							.execute = cmd_clean_execute });

	cli_register((Command){ .name = "test",
							.description = "Discover and run tests",
							.execute = cmd_test_execute });

	cli_register((Command){ .name = "test-add",
							.description = "Scaffold new test module",
							.execute = cmd_test_add_execute });

	// Run CLI with provided arguments
	return cli_run(argc, argv);
}
