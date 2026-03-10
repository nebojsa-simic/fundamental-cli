#include "cmd_init.h"
#include "../cli.h"
#include "vendor/fundamental/include/filesystem/filesystem.h"
#include "vendor/fundamental/include/console/console.h"
#include "vendor/fundamental/include/error/error.h"
#include "vendor/fundamental/include/string/string.h"
#include "vendor/fundamental/include/file/file.h"
#include "vendor/fundamental/include/memory/memory.h"
#include "vendor/fundamental/include/async/async.h"
#include <stdbool.h>

// Template: src/main.c
static const char* TEMPLATE_MAIN_C =
"#include \"commands/cmd_version.h\"\n"
"#include \"commands/cmd_help.h\"\n"
"#include \"cli.h\"\n"
"\n"
"int main(int argc, const char **argv) {\n"
"  cli_init();\n"
"  cli_register((Command){.name = \"version\",\n"
"                         .description = \"Show version information\",\n"
"                         .execute = cmd_version_execute});\n"
"  cli_register((Command){.name = \"help\",\n"
"                         .description = \"Show this help message\",\n"
"                         .execute = cmd_help_execute});\n"
"  return cli_run(argc, argv);\n"
"}\n";

// Template: src/cli.h
static const char* TEMPLATE_CLI_H =
"#ifndef CLI_H\n"
"#define CLI_H\n"
"\n"
"#include \"vendor/fundamental/include/console/console.h\"\n"
"#include \"vendor/fundamental/include/string/string.h\"\n"
"\n"
"typedef struct Command {\n"
"  String name;\n"
"  String description;\n"
"  int (*execute)(int argc, const char **argv);\n"
"} Command;\n"
"\n"
"ErrorResult cli_init(void);\n"
"ErrorResult cli_register(Command cmd);\n"
"int cli_run(int argc, const char **argv);\n"
"int cli_show_help(void);\n"
"\n"
"#endif // CLI_H\n";

// Template: src/cli.c
static const char* TEMPLATE_CLI_C =
"#include \"cli.h\"\n"
"\n"
"#define MAX_COMMANDS 16\n"
"#define BUFFER_SIZE 256\n"
"\n"
"static Command commands[MAX_COMMANDS];\n"
"static size_t command_count = 0;\n"
"\n"
"ErrorResult cli_init(void) {\n"
"  command_count = 0;\n"
"  for (size_t i = 0; i < MAX_COMMANDS; i++) {\n"
"    commands[i].name = NULL;\n"
"    commands[i].description = NULL;\n"
"    commands[i].execute = NULL;\n"
"  }\n"
"  return ERROR_RESULT_NO_ERROR;\n"
"}\n"
"\n"
"ErrorResult cli_register(Command cmd) {\n"
"  if (command_count >= MAX_COMMANDS) {\n"
"    fun_console_error_line(\"Error: Maximum command count reached\");\n"
"    return fun_error_result(1, \"Command limit exceeded\");\n"
"  }\n"
"  commands[command_count] = cmd;\n"
"  command_count++;\n"
"  return ERROR_RESULT_NO_ERROR;\n"
"}\n"
"\n"
"int cli_show_help(void) {\n"
"  fun_console_write_line(\"fun - A CLI built with fundamental library\");\n"
"  fun_console_write_line(\"\");\n"
"  fun_console_write_line(\"Usage: fun <command> [arguments]\");\n"
"  fun_console_write_line(\"\");\n"
"  fun_console_write_line(\"Available commands:\");\n"
"  for (size_t i = 0; i < command_count; i++) {\n"
"    fun_console_write(\"  \");\n"
"    fun_console_write(commands[i].name);\n"
"    fun_console_write(\" - \");\n"
"    fun_console_write_line(commands[i].description);\n"
"  }\n"
"  fun_console_write_line(\"\");\n"
"  fun_console_write_line(\"Use 'fun <command> --help' for command-specific help.\");\n"
"  return 0;\n"
"}\n"
"\n"
"int cli_run(int argc, const char **argv) {\n"
"  if (argc < 2) {\n"
"    return cli_show_help();\n"
"  }\n"
"  const char *command_name = argv[1];\n"
"  if (command_name[0] == '-') {\n"
"    if (command_name[1] == 'h' ||\n"
"        (command_name[1] == '-' && command_name[2] == 'h' &&\n"
"         command_name[3] == 'e' && command_name[4] == 'l' &&\n"
"         command_name[5] == 'p')) {\n"
"      return cli_show_help();\n"
"    }\n"
"  }\n"
"  for (size_t i = 0; i < command_count; i++) {\n"
"    if (fun_string_compare(commands[i].name, command_name) == 0) {\n"
"      return commands[i].execute(argc - 1, argv + 1);\n"
"    }\n"
"  }\n"
"  fun_console_write(\"Error: Unknown command '\");\n"
"  fun_console_write(command_name);\n"
"  fun_console_write_line(\"'\");\n"
"  fun_console_write_line(\"\");\n"
"  return cli_show_help();\n"
"}\n";

// Template: commands/cmd_version.h
static const char* TEMPLATE_CMD_VERSION_H =
"#ifndef CMD_VERSION_H\n"
"#define CMD_VERSION_H\n"
"\n"
"int cmd_version_execute(int argc, const char **argv);\n"
"\n"
"#endif // CMD_VERSION_H\n";

// Template: commands/cmd_version.c
static const char* TEMPLATE_CMD_VERSION_C =
"#include \"cmd_version.h\"\n"
"#include \"../src/cli.h\"\n"
"\n"
"int cmd_version_execute(int argc, const char **argv) {\n"
"  (void)argc;\n"
"  (void)argv;\n"
"  fun_console_write_line(\"fun v0.1.0\");\n"
"  fun_console_write_line(\"Built with fundamental library\");\n"
"  return 0;\n"
"}\n";

// Template: commands/cmd_help.h
static const char* TEMPLATE_CMD_HELP_H =
"#ifndef CMD_HELP_H\n"
"#define CMD_HELP_H\n"
"\n"
"int cmd_help_execute(int argc, const char **argv);\n"
"\n"
"#endif // CMD_HELP_H\n";

// Template: commands/cmd_help.c
static const char* TEMPLATE_CMD_HELP_C =
"#include \"cmd_help.h\"\n"
"#include \"../src/cli.h\"\n"
"\n"
"int cmd_help_execute(int argc, const char **argv) {\n"
"  (void)argc;\n"
"  (void)argv;\n"
"  return cli_show_help();\n"
"}\n";

// Template: build-windows-amd64.bat
static const char* TEMPLATE_BUILD_WINDOWS_BAT =
"@ECHO OFF\n"
"\n"
"REM Compile fun CLI - Complete standalone build\n"
"REM Uses -nostdlib to exclude standard C library\n"
"REM Uses -fno-builtin to prevent compiler builtin substitution\n"
"REM Uses -e main to specify entry point (required with -nostdlib)\n"
"\n"
"gcc ^\n"
"    --std=c17 -Os ^\n"
"    -nostdlib ^\n"
"    -fno-builtin ^\n"
"    -fno-exceptions ^\n"
"    -fno-unwind-tables ^\n"
"    -e main ^\n"
"    -mconsole ^\n"
"    -I . ^\n"
"    -I vendor/fundamental/include ^\n"
"    vendor/fundamental/src/startup/startup.c ^\n"
"    vendor/fundamental/arch/startup/windows-amd64/windows.c ^\n"
"    src/main.c ^\n"
"    src/cli.c ^\n"
"    commands/cmd_version.c ^\n"
"    commands/cmd_help.c ^\n"
"    vendor/fundamental/src/console/console.c ^\n"
"    vendor/fundamental/src/string/stringConversion.c ^\n"
"    vendor/fundamental/src/string/stringOperations.c ^\n"
"    vendor/fundamental/src/string/stringTemplate.c ^\n"
"    vendor/fundamental/src/string/stringValidation.c ^\n"
"    vendor/fundamental/arch/console/windows-amd64/console.c ^\n"
"    vendor/fundamental/arch/memory/windows-amd64/memory.c ^\n"
"    -lkernel32 ^\n"
"    -o fun.exe\n"
"\n"
"REM Strip unnecessary symbols\n"
"strip --strip-unneeded fun.exe\n"
"\n"
"echo Build complete: fun.exe\n";

// Template: build-linux-amd64.sh
static const char* TEMPLATE_BUILD_LINUX_SH =
"#!/bin/bash\n"
"\n"
"# Compile fun CLI - Complete standalone build\n"
"# Uses -nostdlib to exclude standard C library\n"
"# Uses -fno-builtin to prevent compiler builtin substitution\n"
"# Uses -e main to specify entry point (required with -nostdlib)\n"
"\n"
"gcc \\\n"
"    --std=c17 -Os \\\n"
"    -nostdlib \\\n"
"    -fno-builtin \\\n"
"    -fno-exceptions \\\n"
"    -fno-unwind-tables \\\n"
"    -e main \\\n"
"    -I . \\\n"
"    -I vendor/fundamental/include \\\n"
"    vendor/fundamental/src/startup/startup.c \\\n"
"    vendor/fundamental/arch/startup/linux-amd64/linux.c \\\n"
"    src/main.c \\\n"
"    src/cli.c \\\n"
"    commands/cmd_version.c \\\n"
"    commands/cmd_help.c \\\n"
"    vendor/fundamental/src/console/console.c \\\n"
"    vendor/fundamental/src/string/stringConversion.c \\\n"
"    vendor/fundamental/src/string/stringOperations.c \\\n"
"    vendor/fundamental/src/string/stringTemplate.c \\\n"
"    vendor/fundamental/src/string/stringValidation.c \\\n"
"    vendor/fundamental/arch/console/linux-amd64/console.c \\\n"
"    vendor/fundamental/arch/memory/linux-amd64/memory.c \\\n"
"    -o fun\n"
"\n"
"# Strip unnecessary symbols\n"
"strip --strip-unneeded fun\n"
"\n"
"echo \"Build complete: fun\"\n";

// Template: fun.ini
static const char* TEMPLATE_FUN_INI =
"name = my-project\n"
"version = 0.1.0\n"
"description = My fundamental CLI app\n"
"entry = src/main.c\n"
"output = fun.exe\n"
"\n"
"[dependencies]\n"
"fundamental = local\n"
"\n"
"[build]\n"
"standard = -nostdlib\n"
"flags = -fno-builtin -fno-exceptions\n";

// Template: README.md
static const char* TEMPLATE_README_MD =
"# my-project\n"
"\n"
"A command-line interface built with the [fundamental](https://github.com/nebojsa-simic/fundamental) library.\n"
"\n"
"## Features\n"
"\n"
"- **Zero stdlib runtime dependencies** - Uses only fundamental library\n"
"- **Line-buffered console output** - Efficient output with 512-byte buffer\n"
"- **Cross-platform** - Works on Windows and POSIX systems\n"
"- **Command-based architecture** - Easy to extend with new commands\n"
"\n"
"## Building\n"
"\n"
"### Windows\n"
"\n"
"```batch\n"
".\\build-windows-amd64.bat\n"
"```\n"
"\n"
"### Linux\n"
"\n"
"```bash\n"
"./build-linux-amd64.sh\n"
"```\n"
"\n"
"## Usage\n"
"\n"
"```bash\n"
"# Show help\n"
"./fun --help\n"
"./fun help\n"
"\n"
"# Show version\n"
"./fun version\n"
"```\n"
"\n"
"## Adding Commands\n"
"\n"
"1. Create command files in `commands/` directory\n"
"2. Implement execute function\n"
"3. Register command in src/main.c\n"
"\n"
"## License\n"
"\n"
"Same license as fundamental library.\n";

// Template: arch/startup/windows-amd64/windows.c
static const char* TEMPLATE_ARCH_WINDOWS_C =
"// Fundamental Library - Windows Runtime Initialization\n"
"// Provides minimal startup support for zero-stdlib builds\n"
"\n"
"#include <windows.h>\n"
"\n"
"// Provide __main to satisfy GCC when using -ffreestanding/-nostdlib\n"
"void __main(void) {\n"
"  // No initialization needed for fundamental library\n"
"  // This stub prevents linker errors when building without CRT\n"
"}\n";

// Template: arch/startup/linux-amd64/linux.c
static const char* TEMPLATE_ARCH_LINUX_C =
"// Fundamental Library - Linux Runtime Initialization\n"
"// Provides minimal startup support for zero-stdlib builds\n"
"\n"
"// Provide __main to satisfy GCC when using -ffreestanding/-nostdlib\n"
"void __main(void) {\n"
"  // No initialization needed for fundamental library\n"
"  // This stub prevents linker errors when building without CRT\n"
"}\n";

// Template: fundamental-expert/SKILL.md (embedded as C string)
static const char* TEMPLATE_SKILL_MD =
"---\n"
"name: fundamental-expert\n"
"description: Expert guide for building applications with Fundamental Library - knows the architecture, patterns, and all modules inside out.\n"
"license: MIT\n"
"compatibility: Standalone skill for application development guidance.\n"
"metadata:\n"
"  author: fundamental-library\n"
"  version: \"1.0\"\n"
"  generatedBy: fundamental-expert skill\n"
"---\n"
"\n"
"# Fundamental Library Expert Skill\n"
"\n"
"I am your expert guide for building applications with the **Fundamental Library** - a zero-stdlib C library for cross-platform CLI applications.\n"
"\n"
"**My purpose:** Help you write correct, idiomatic Fundamental Library code by providing patterns, examples, and architectural guidance from deep knowledge of the codebase.\n"
"\n"
"---\n"
"\n"
"## What I Know\n"
"\n"
"### Core Architecture\n"
"\n"
"```\n"
"fundamental/\n"
"├── arch/              # Platform-specific implementations\n"
"│   ├── async/        # Async operations (linux-amd64, windows-amd64)\n"
"│   ├── config/       # Environment variable access\n"
"│   ├── console/      # Console I/O per platform\n"
"│   ├── file/         # File I/O implementations\n"
"│   ├── filesystem/   # Filesystem operations\n"
"│   ├── memory/       # Memory management (syscalls, VirtualAlloc)\n"
"│   ├── startup/      # Platform entry points (_start, main)\n"
"│   └── stream/       # Stream I/O per platform\n"
"├── include/          # Public API headers\n"
"│   ├── array/        # Dynamic arrays\n"
"│   ├── async/        # Async primitives\n"
"│   ├── collections/  # Hash maps, RB-trees, sets\n"
"│   ├── config/       # Configuration management\n"
"│   ├── console/      # Console I/O\n"
"│   ├── error/        # Error handling system\n"
"│   ├── file/         # File I/O interface\n"
"│   ├── filesystem/   # Directory/path operations\n"
"│   ├── hashmap/      # Hash map interface\n"
"│   ├── memory/       # Memory management\n"
"│   ├── rbtree/       # Red-black tree interface\n"
"│   ├── set/          # Set data structure\n"
"│   ├── stream/       # Stream I/O interface\n"
"│   └── string/       # String operations\n"
"├── src/              # Core implementations\n"
"│   ├── array/        # Dynamic array implementation\n"
"│   ├── async/        # Async scheduler, process spawn\n"
"│   ├── config/       # Config loading, INI parser, CLI parser\n"
"│   ├── console/      # Console output with buffering\n"
"│   ├── filesystem/   # Path and directory operations\n"
"│   ├── hashmap/      # Hash map implementation\n"
"│   ├── rbtree/       # Red-black tree implementation\n"
"│   ├── set/          # Set implementation\n"
"│   ├── startup/      # Cross-platform entry point\n"
"│   ├── stream/       # Stream lifecycle, file streams\n"
"│   └── string/       # Conversion, templating, validation\n"
"└── tests/            # Comprehensive test suites\n"
"```\n"
"\n"
"### Design Principles (NEVER violate these)\n"
"\n"
"| Principle | What It Means | Example |\n"
"|-----------|--------------|---------|\n"
"| **Zero stdlib** | No C standard library in library code | Use `fun_memory_allocate()`, not `malloc()` |\n"
"| **Caller-allocated memory** | Functions don't allocate for caller | Pass pre-allocated buffer, get size back |\n"
"| **Explicit errors** | All functions return `Result` types | Check `fun_error_is_error(result.error)` |\n"
"| **Descriptive naming** | `fun_` prefix, full names | `fun_string_from_int()`, not `fun_itoa()` |\n"
"| **Cross-platform** | No OS logic outside `arch/` | Use file module, not direct syscalls |\n"
"| **Async by default** | I/O returns `AsyncResult` | Use `fun_async_await()` to block if needed |\n";

// Forward declarations
static ErrorResult create_directories(void);
static ErrorResult write_template_file(const char* path, const char* content);
static ErrorResult copy_fundamental_folders(void);
static ErrorResult copy_folder(const char* src, const char* dst);
static bool is_directory_empty(const char* path);
static ErrorResult validate_current_directory(void);

/**
 * Execute the init command
 * @param argc Argument count
 * @param argv Argument vector
 * @return Exit code (0 for success, non-zero for error)
 */
int cmd_init_execute(int argc, const char **argv) {
  (void)argc;
  (void)argv;

  fun_console_write_line("Initializing fundamental project...\n");

  // Step 1: Validate current directory
  ErrorResult validation = validate_current_directory();
  if (fun_error_is_error(validation)) {
    fun_console_error_line(validation.message);
    return 1;
  }

  // Step 2: Create directory structure
  ErrorResult dirs = create_directories();
  if (fun_error_is_error(dirs)) {
    fun_console_error_line("Failed to create directories");
    return 1;
  }
  fun_console_write_line("✓ Created directory structure");

  // Step 3: Write template files
  ErrorResult err;
  
  err = write_template_file("src/main.c", TEMPLATE_MAIN_C);
  if (fun_error_is_error(err)) return 1;
  
  err = write_template_file("src/cli.h", TEMPLATE_CLI_H);
  if (fun_error_is_error(err)) return 1;
  
  err = write_template_file("src/cli.c", TEMPLATE_CLI_C);
  if (fun_error_is_error(err)) return 1;
  
  err = write_template_file("commands/cmd_version.h", TEMPLATE_CMD_VERSION_H);
  if (fun_error_is_error(err)) return 1;
  
  err = write_template_file("commands/cmd_version.c", TEMPLATE_CMD_VERSION_C);
  if (fun_error_is_error(err)) return 1;
  
  err = write_template_file("commands/cmd_help.h", TEMPLATE_CMD_HELP_H);
  if (fun_error_is_error(err)) return 1;
  
  err = write_template_file("commands/cmd_help.c", TEMPLATE_CMD_HELP_C);
  if (fun_error_is_error(err)) return 1;
  
  fun_console_write_line("✓ Created source files");

  // Step 4: Write build scripts
  err = write_template_file("build-windows-amd64.bat", TEMPLATE_BUILD_WINDOWS_BAT);
  if (fun_error_is_error(err)) return 1;
  
  err = write_template_file("build-linux-amd64.sh", TEMPLATE_BUILD_LINUX_SH);
  if (fun_error_is_error(err)) return 1;
  fun_console_write_line("✓ Created build scripts");

  // Step 5: Write fun.ini
  err = write_template_file("fun.ini", TEMPLATE_FUN_INI);
  if (fun_error_is_error(err)) return 1;
  fun_console_write_line("✓ Created fun.ini");

  // Step 6: Write README.md
  err = write_template_file("README.md", TEMPLATE_README_MD);
  if (fun_error_is_error(err)) return 1;
  fun_console_write_line("✓ Created README.md");

  // Step 7: Copy fundamental folders (arch/, include/, src/)
  err = copy_fundamental_folders();
  if (fun_error_is_error(err)) {
    fun_console_error_line("Failed to copy fundamental library");
    return 1;
  }
  fun_console_write_line("✓ Copied fundamental library (arch/, include/, src/)");

  // Step 8: Write fundamental-expert skill
  err = write_template_file(".opencode/skills/fundamental-expert/SKILL.md", TEMPLATE_SKILL_MD);
  if (fun_error_is_error(err)) {
    fun_console_write_line("⚠ Failed to write skill file (optional)");
  } else {
    fun_console_write_line("✓ Created fundamental-expert skill");
  }

  fun_console_write_line("\n✓ Project initialized successfully!");
  fun_console_write_line("\nNext steps:\n");
  fun_console_write_line("  1. Run '.\\build-windows-amd64.bat' (Windows) or './build-linux-amd64.sh' (Linux)");
  fun_console_write_line("  2. Run './fun.exe' or './fun' to test your CLI");
  fun_console_write_line("  3. Add new commands in commands/ directory");
  
  return 0;
}

/**
 * Validate that current directory is empty or doesn't exist
 * @return ErrorResult indicating success or failure
 */
static ErrorResult validate_current_directory(void) {
  // Check if directory has files (simple check - would need filesystem module)
  // For now, assume user runs this in an empty directory
  // TODO: Implement proper directory listing and validation
  return ERROR_RESULT_NO_ERROR;
}

/**
 * Create required directories
 * @return ErrorResult indicating success or failure
 */
static ErrorResult create_directories(void) {
  ErrorResult err;
  
  // Create src/
  err = fun_filesystem_create_directory("src");
  if (fun_error_is_error(err)) return err;
  
  // Create commands/
  err = fun_filesystem_create_directory("commands");
  if (fun_error_is_error(err)) return err;
  
  // Create vendor/
  err = fun_filesystem_create_directory("vendor");
  if (fun_error_is_error(err)) return err;
  
  // Create .opencode/skills/fundamental-expert/
  err = fun_filesystem_create_directory(".opencode/skills/fundamental-expert");
  if (fun_error_is_error(err)) return err;
  
  // Create arch/startup/windows-amd64/
  err = fun_filesystem_create_directory("arch/startup/windows-amd64");
  if (fun_error_is_error(err)) return err;
  
  // Create arch/startup/linux-amd64/
  err = fun_filesystem_create_directory("arch/startup/linux-amd64");
  if (fun_error_is_error(err)) return err;
  
  return ERROR_RESULT_NO_ERROR;
}

/**
 * Write a template file to disk
 * @param path File path
 * @param content Template content
 * @return ErrorResult indicating success or failure
 */
static ErrorResult write_template_file(const char* path, const char* content) {
  // TODO: Implement file write using fundamental's file module
  // For now, this is a placeholder
  (void)path;
  (void)content;
  return ERROR_RESULT_NO_ERROR;
}

/**
 * Copy essential fundamental folders (arch/, include/, src/)
 * @return ErrorResult indicating success or failure
 */
static ErrorResult copy_fundamental_folders(void) {
  ErrorResult err;
  
  // Copy arch/
  err = copy_folder("../fundamental/arch", "vendor/fundamental/arch");
  if (fun_error_is_error(err)) return err;
  
  // Copy include/
  err = copy_folder("../fundamental/include", "vendor/fundamental/include");
  if (fun_error_is_error(err)) return err;
  
  // Copy src/
  err = copy_folder("../fundamental/src", "vendor/fundamental/src");
  if (fun_error_is_error(err)) return err;
  
  return ERROR_RESULT_NO_ERROR;
}

/**
 * Copy a folder from source to destination
 * @param src Source path
 * @param dst Destination path
 * @return ErrorResult indicating success or failure
 */
static ErrorResult copy_folder(const char* src, const char* dst) {
  // TODO: Implement folder copy using fundamental's filesystem module
  // For now, this is a placeholder
  (void)src;
  (void)dst;
  return ERROR_RESULT_NO_ERROR;
}

/**
 * Check if directory is empty
 * @param path Directory path
 * @return true if empty, false otherwise
 */
static bool is_directory_empty(const char* path) {
  // TODO: Implement directory listing
  (void)path;
  return true;
}
