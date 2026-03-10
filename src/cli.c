#include "cli.h"

#define MAX_COMMANDS 16
#define BUFFER_SIZE 256

static Command commands[MAX_COMMANDS];
static size_t command_count = 0;

ErrorResult cli_init(void)
{
	command_count = 0;
	for (size_t i = 0; i < MAX_COMMANDS; i++) {
		commands[i].name = NULL;
		commands[i].description = NULL;
		commands[i].execute = NULL;
	}
	return ERROR_RESULT_NO_ERROR;
}

ErrorResult cli_register(Command cmd)
{
	if (command_count >= MAX_COMMANDS) {
		fun_console_error_line("Error: Maximum command count reached");
		return fun_error_result(1, "Command limit exceeded");
	}

	commands[command_count] = cmd;
	command_count++;

	return ERROR_RESULT_NO_ERROR;
}

int cli_show_help(void)
{
	fun_console_write_line("fun - A CLI built with fundamental library");
	fun_console_write_line("");
	fun_console_write_line("Usage: fun <command> [arguments]");
	fun_console_write_line("");
	fun_console_write_line("Available commands:");

	for (size_t i = 0; i < command_count; i++) {
		fun_console_write("  ");
		fun_console_write(commands[i].name);
		fun_console_write(" - ");
		fun_console_write_line(commands[i].description);
	}

	fun_console_write_line("");
	fun_console_write_line(
		"Use 'fun <command> --help' for command-specific help.");

	return 0;
}
int cli_run(int argc, const char **argv)
{
	if (argc < 2) {
		return cli_show_help();
	}

	const char *command_name = argv[1];

	// Check for help flags explicitly
	if (command_name[0] == '-') {
		if (command_name[1] == 'h' ||
			(command_name[1] == '-' && command_name[2] == 'h' &&
			 command_name[3] == 'e' && command_name[4] == 'l' &&
			 command_name[5] == 'p')) {
			return cli_show_help();
		}
	}

	// Find and execute the command
	for (size_t i = 0; i < command_count; i++) {
		if (fun_string_compare(commands[i].name, command_name) == 0) {
			// Pass remaining arguments to command (skip command name)
			return commands[i].execute(argc - 1, argv + 1);
		}
	}

	// Command not found
	fun_console_write("Error: Unknown command '");
	fun_console_write(command_name);
	fun_console_write_line("'");
	fun_console_write_line("");
	return cli_show_help();
}