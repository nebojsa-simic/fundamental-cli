#include "cmd_init.h"
#include "src/cli.h"
#include "vendor/fundamental/include/async/async.h"
#include "vendor/fundamental/include/console/console.h"
#include "vendor/fundamental/include/file/file.h"
#include "vendor/fundamental/include/filesystem/filesystem.h"
#include "vendor/fundamental/include/memory/memory.h"
#include "vendor/fundamental/include/string/string.h"

static const char *T_MAIN =
	"#include \"src/commands/cmd_version.h\"\n#include "
	"\"src/commands/cmd_help.h\"\n#include \"cli.h\"\n\nint cli_main(int argc, "
	"const char **argv) {\n  cli_init();\n  cli_register((Command){.name = "
	"\"version\", .description = \"Show version information\", .execute = "
	"cmd_version_execute});\n  cli_register((Command){.name = \"help\", "
	".description = \"Show this help message\", .execute = "
	"cmd_help_execute});\n  return cli_run(argc, argv);\n}\n";
static const char *T_CLI_H =
	"#ifndef CLI_H\n#define CLI_H\n#include "
	"\"vendor/fundamental/include/console/console.h\"\n#include "
	"\"vendor/fundamental/include/string/string.h\"\ntypedef struct Command { "
	"const char* name; const char* description; int (*execute)(int argc, const "
	"char **argv); } Command;\nErrorResult cli_init(void); ErrorResult "
	"cli_register(Command cmd); int cli_run(int argc, const char **argv); int "
	"cli_show_help(void);\n#endif\n";
static const char *T_CLI_C =
	"#include \"cli.h\"\n#include "
	"\"vendor/fundamental/include/console/console.h\"\n#include "
	"\"vendor/fundamental/include/string/string.h\"\n#include "
	"\"vendor/fundamental/include/error/error.h\"\n#define MAX_COMMANDS "
	"16\nstatic Command commands[MAX_COMMANDS]; static size_t command_count = "
	"0;\nErrorResult cli_init(void) { command_count = 0; for (size_t i = 0; i "
	"< MAX_COMMANDS; i++) { commands[i].name = NULL; commands[i].description = "
	"NULL; commands[i].execute = NULL; } return ERROR_RESULT_NO_ERROR; "
	"}\nErrorResult cli_register(Command cmd) { if (command_count >= "
	"MAX_COMMANDS) { fun_console_error_line(\"Error: Maximum command count "
	"reached\"); return fun_error_result(1, \"Command limit exceeded\"); } "
	"commands[command_count] = cmd; command_count++; return "
	"ERROR_RESULT_NO_ERROR; }\nint cli_show_help(void) { "
	"fun_console_write_line(\"app - A CLI built with fundamental library\"); "
	"fun_console_write_line(\"\"); fun_console_write_line(\"Usage: app "
	"<command> [arguments]\"); fun_console_write_line(\"\"); "
	"fun_console_write_line(\"Available commands:\"); for (size_t i = 0; i < "
	"command_count; i++) { fun_console_write(\"  \"); "
	"fun_console_write(commands[i].name); fun_console_write(\" - \"); "
	"fun_console_write_line(commands[i].description); } "
	"fun_console_write_line(\"\"); return 0; }\nint cli_run(int argc, const "
	"char **argv) { if (argc < 2) return cli_show_help(); const char "
	"*command_name = argv[1]; if (command_name[0] == '-') { if "
	"(command_name[1] == 'h' || (command_name[1] == '-' && command_name[2] == "
	"'h' && command_name[3] == 'e' && command_name[4] == 'l' && "
	"command_name[5] == 'p')) return cli_show_help(); } for (size_t i = 0; i < "
	"command_count; i++) { if (fun_string_compare(commands[i].name, "
	"command_name) == 0) return commands[i].execute(argc - 1, argv + 1); } "
	"fun_console_write(\"Error: Unknown command '\"); "
	"fun_console_write(command_name); fun_console_write_line(\"'\"); "
	"fun_console_write_line(\"\"); return cli_show_help(); }\n";
static const char *T_VH =
	"#ifndef CMD_VERSION_H\n#define CMD_VERSION_H\nint cmd_version_execute(int "
	"argc, const char **argv);\n#endif\n";
static const char *T_VC =
	"#include \"cmd_version.h\"\n#include \"src/cli.h\"\nint "
	"cmd_version_execute(int argc, const char **argv) { (void)argc; "
	"(void)argv; fun_console_write_line(\"app v0.1.0\"); "
	"fun_console_write_line(\"Built with fundamental library\"); return 0; }\n";
static const char *T_HH =
	"#ifndef CMD_HELP_H\n#define CMD_HELP_H\nint cmd_help_execute(int argc, "
	"const char **argv);\n#endif\n";
static const char *T_HC =
	"#include \"cmd_help.h\"\n#include \"src/cli.h\"\nint "
	"cmd_help_execute(int argc, const char **argv) { (void)argc; (void)argv; "
	"return cli_show_help(); }\n";
static const char *T_BAT =
	"@ECHO OFF\nREM Compile app CLI\nif not exist build mkdir build\n\ngcc "
	"--std=c17 -Os -nostdlib -fno-builtin "
	"-fno-exceptions -fno-unwind-tables -e main -mconsole -I . -I "
	"vendor/fundamental/include vendor/fundamental/src/startup/startup.c "
	"vendor/fundamental/arch/startup/windows-amd64/windows.c src/main.c "
	"src/cli.c src/commands/cmd_version.c src/commands/cmd_help.c "
	"vendor/fundamental/src/console/console.c "
	"vendor/fundamental/src/string/stringConversion.c "
	"vendor/fundamental/src/string/stringOperations.c "
	"vendor/fundamental/src/string/stringTemplate.c "
	"vendor/fundamental/src/string/stringValidation.c "
	"vendor/fundamental/arch/console/windows-amd64/console.c "
	"vendor/fundamental/arch/memory/windows-amd64/memory.c -lkernel32 -o "
	"build/app-windows-amd64.exe\nstrip --strip-unneeded "
	"build/app-windows-amd64.exe\necho Build complete: "
	"build/app-windows-amd64.exe\n";
static const char *T_SH =
	"#!/bin/bash\n# Compile app CLI\nmkdir -p build\n\ngcc --std=c17 -Os "
	"-nostdlib -fno-builtin "
	"-fno-exceptions -fno-unwind-tables -e main -I . -I "
	"vendor/fundamental/include vendor/fundamental/src/startup/startup.c "
	"vendor/fundamental/arch/startup/linux-amd64/linux.c src/main.c src/cli.c "
	"src/commands/cmd_version.c src/commands/cmd_help.c "
	"vendor/fundamental/src/console/console.c "
	"vendor/fundamental/src/string/stringConversion.c "
	"vendor/fundamental/src/string/stringOperations.c "
	"vendor/fundamental/src/string/stringTemplate.c "
	"vendor/fundamental/src/string/stringValidation.c "
	"vendor/fundamental/arch/console/linux-amd64/console.c "
	"vendor/fundamental/arch/memory/linux-amd64/memory.c -o "
	"build/app-linux-amd64\nstrip --strip-unneeded "
	"build/app-linux-amd64\necho Build complete: build/app-linux-amd64\n";
static const char *T_INI =
	"name = my-project\nversion = 0.1.0\ndescription = My fundamental CLI "
	"app\nentry = src/main.c\noutput = app.exe\n\n[dependencies]\nfundamental "
	"= local\n\n[build]\nstandard = -nostdlib\nflags = -fno-builtin "
	"-fno-exceptions\n";
static const char *T_README =
	"# my-project\n\nA CLI built with the "
	"[fundamental](https://github.com/nebojsa-simic/fundamental) "
	"library.\n\n## Building\n\n### "
	"Windows\n```batch\n.\\build-windows-amd64.bat\n```\n\n### "
	"Linux\n```bash\n./build-linux-amd64.sh\n```\n\n## Usage\n```bash\n./build/app-windows-amd64.exe "
	"--help\n./build/app-linux-amd64 --help\n```\n\n## License\nSame as fundamental library.\n";
static const char *T_ARCH = "void __main(void) {}\n";
static const char *T_SKILL =
	"---\nname: fundamental-expert\ndescription: Expert guide for building "
	"applications with Fundamental Library\n---\n\n# Fundamental Library "
	"Expert Skill\n\nI am your expert guide for building applications with the "
	"**Fundamental Library**.\n\n## Design Principles\n\n- **Zero stdlib**: No "
	"C standard library\n- **Caller-allocated memory**: Functions don't "
	"allocate for caller\n- **Explicit errors**: All functions return Result "
	"types\n- **Cross-platform**: No OS logic outside arch/\n";

static ErrorResult write_file(const char *path, const char *content)
{
	StringLength len = fun_string_length(content);
	MemoryResult mem_result = fun_memory_allocate(len + 1);
	if (fun_error_is_error(mem_result.error))
		return mem_result.error;
	fun_string_copy(content, (char *)mem_result.value, len + 1);
	AsyncResult write_result =
		fun_write_memory_to_file((Write){ .file_path = (String)path,
										  .input = mem_result.value,
										  .bytes_to_write = len });
	fun_async_await(&write_result, -1);
	voidResult free_result = fun_memory_free(&mem_result.value);
	(void)free_result;
	return (write_result.status != ASYNC_COMPLETED) ?
			   fun_error_result(1, "Failed") :
			   ERROR_RESULT_NO_ERROR;
}

int cmd_init_execute(int argc, const char **argv)
{
	(void)argc;
	(void)argv;
	fun_console_write_line("Initializing fundamental project...\n");

	fun_console_write_line("Creating directories...\n");
	fun_filesystem_create_directory((String) "src");
	fun_filesystem_create_directory((String) "src/commands");
	fun_filesystem_create_directory(
		(String) ".opencode/skills/fundamental-expert");
	fun_filesystem_create_directory((String) "arch/startup/windows-amd64");
	fun_filesystem_create_directory((String) "arch/startup/linux-amd64");
	fun_console_write_line("✓ Created directories\n");

	fun_console_write_line("Writing source files...\n");
	write_file("src/main.c", T_MAIN);
	write_file("src/cli.h", T_CLI_H);
	write_file("src/cli.c", T_CLI_C);
	write_file("src/commands/cmd_version.h", T_VH);
	write_file("src/commands/cmd_version.c", T_VC);
	write_file("src/commands/cmd_help.h", T_HH);
	write_file("src/commands/cmd_help.c", T_HC);
	fun_console_write_line("✓ Created source files\n");

	fun_console_write_line("Writing build scripts...\n");
	write_file("build-windows-amd64.bat", T_BAT);
	write_file("build-linux-amd64.sh", T_SH);
	fun_console_write_line("✓ Created build scripts\n");

	fun_console_write_line("Writing configuration...\n");
	write_file("fun.ini", T_INI);
	write_file("README.md", T_README);
	fun_console_write_line("✓ Created configuration\n");

	fun_console_write_line("Writing arch files...\n");
	write_file("arch/startup/windows-amd64/windows.c", T_ARCH);
	write_file("arch/startup/linux-amd64/linux.c", T_ARCH);
	fun_console_write_line("✓ Created arch files\n");

	fun_console_write_line("Writing fundamental-expert skill...\n");
	write_file(".opencode/skills/fundamental-expert/SKILL.md", T_SKILL);
	fun_console_write_line("✓ Created skill\n");

	fun_console_write_line("\n================================\n");
	fun_console_write_line("✓ Project initialized successfully!\n");
	fun_console_write_line("================================\n\n");
	fun_console_write_line("Next steps:\n\n");
	fun_console_write_line("  1. Copy fundamental library:\n");
	fun_console_write_line(
		"     Windows: xcopy /E /I ..\\fundamental vendor\\fundamental\n");
	fun_console_write_line(
		"     Linux:   cp -r ../fundamental vendor/fundamental\n\n");
	fun_console_write_line(
		"  2. Build: .\\build-windows-amd64.bat or ./build-linux-amd64.sh\n\n");
	fun_console_write_line("  3. Run: .\\app.exe or ./app\n\n");

	return 0;
}
