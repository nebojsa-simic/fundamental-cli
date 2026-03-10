#include "cmd_version.h"
#include "../src/cli.h"

int cmd_version_execute(int argc, const char **argv)
{
	(void)argc;
	(void)argv;

	fun_console_write_line("fun v0.1.0");
	fun_console_write_line("Built with fundamental library v0.1.0");
	fun_console_write_line("");
	fun_console_write_line("Features:");
	fun_console_write_line("  - Zero stdlib runtime dependencies");
	fun_console_write_line("  - Line-buffered console output");
	fun_console_write_line("  - Cross-platform (Windows, POSIX)");

	return 0;
}
