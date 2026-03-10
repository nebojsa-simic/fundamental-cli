#include "cmd_help.h"
#include "../src/cli.h"

int cmd_help_execute(int argc, const char **argv)
{
	(void)argc;
	(void)argv;

	return cli_show_help();
}
