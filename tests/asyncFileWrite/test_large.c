// Test: Async File Write with Large Templates
// Reproduces bug where fun_write_memory_to_file hangs with large strings
// This mimics cmd_init.c which has large static const char* templates

#include "../../vendor/fundamental/include/async/async.h"
#include "../../vendor/fundamental/include/console/console.h"
#include "../../vendor/fundamental/include/error/error.h"
#include "../../vendor/fundamental/include/file/file.h"
#include "../../vendor/fundamental/include/memory/memory.h"
#include "../../vendor/fundamental/include/string/string.h"

// Large template strings (similar to cmd_init.c)
static const char *LARGE_TEMPLATE_1 =
    "#include \"commands/cmd_version.h\"\n"
    "#include \"commands/cmd_help.h\"\n"
    "#include \"cli.h\"\n"
    "\n"
    "int cli_main(int argc, const char **argv) {\n"
    "  // Initialize CLI runtime\n"
    "  cli_init();\n"
    "\n"
    "  // Register available commands\n"
    "  cli_register((Command){.name = \"version\",\n"
    "                         .description = \"Show version information\",\n"
    "                         .execute = cmd_version_execute});\n"
    "\n"
    "  cli_register((Command){.name = \"help\",\n"
    "                         .description = \"Show this help message\",\n"
    "                         .execute = cmd_help_execute});\n"
    "\n"
    "  cli_register((Command){.name = \"init\",\n"
    "                         .description = \"Initialize new fundamental "
    "project\",\n"
    "                         .execute = cmd_init_execute});\n"
    "\n"
    "  // Run CLI with provided arguments\n"
    "  return cli_run(argc, argv);\n"
    "}\n";

static const char *LARGE_TEMPLATE_2 =
    "#ifndef CLI_H\n"
    "#define CLI_H\n"
    "\n"
    "#include \"vendor/fundamental/include/console/console.h\"\n"
    "#include \"vendor/fundamental/include/string/string.h\"\n"
    "\n"
    "typedef struct Command {\n"
    "  const char* name;\n"
    "  const char* description;\n"
    "  int (*execute)(int argc, const char **argv);\n"
    "} Command;\n"
    "\n"
    "ErrorResult cli_init(void);\n"
    "ErrorResult cli_register(Command cmd);\n"
    "int cli_run(int argc, const char **argv);\n"
    "int cli_show_help(void);\n"
    "\n"
    "#endif // CLI_H\n";

static const char *LARGE_TEMPLATE_3 =
    "#include \"cli.h\"\n"
    "#include \"vendor/fundamental/include/console/console.h\"\n"
    "#include \"vendor/fundamental/include/string/string.h\"\n"
    "#include \"vendor/fundamental/include/error/error.h\"\n"
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
    "\n"
    "  commands[command_count] = cmd;\n"
    "  command_count++;\n"
    "\n"
    "  return ERROR_RESULT_NO_ERROR;\n"
    "}\n"
    "\n"
    "int cli_show_help(void) {\n"
    "  fun_console_write_line(\"fun - A CLI built with fundamental "
    "library\");\n"
    "  fun_console_write_line(\"\");\n"
    "  fun_console_write_line(\"Usage: fun <command> [arguments]\");\n"
    "  fun_console_write_line(\"\");\n"
    "  fun_console_write_line(\"Available commands:\");\n"
    "\n"
    "  for (size_t i = 0; i < command_count; i++) {\n"
    "    fun_console_write(\"  \");\n"
    "    fun_console_write(commands[i].name);\n"
    "    fun_console_write(\" - \");\n"
    "    fun_console_write_line(commands[i].description);\n"
    "  }\n"
    "\n"
    "  fun_console_write_line(\"\");\n"
    "  fun_console_write_line(\n"
    "      \"Use 'fun <command> --help' for command-specific help.\");\n"
    "\n"
    "  return 0;\n"
    "}\n"
    "\n"
    "int cli_run(int argc, const char **argv) {\n"
    "  if (argc < 2) {\n"
    "    return cli_show_help();\n"
    "  }\n"
    "\n"
    "  const char *command_name = argv[1];\n"
    "\n"
    "  // Check for help flags explicitly\n"
    "  if (command_name[0] == '-') {\n"
    "    if (command_name[1] == 'h' ||\n"
    "        (command_name[1] == '-' && command_name[2] == 'h' &&\n"
    "         command_name[3] == 'e' && command_name[4] == 'l' &&\n"
    "         command_name[5] == 'p')) {\n"
    "      return cli_show_help();\n"
    "    }\n"
    "  }\n"
    "\n"
    "  // Find and execute the command\n"
    "  for (size_t i = 0; i < command_count; i++) {\n"
    "    if (fun_string_compare(commands[i].name, command_name) == 0) {\n"
    "      // Pass remaining arguments to command (skip command name)\n"
    "      return commands[i].execute(argc - 1, argv + 1);\n"
    "    }\n"
    "  }\n"
    "\n"
    "  // Command not found\n"
    "  fun_console_write(\"Error: Unknown command '\");\n"
    "  fun_console_write(command_name);\n"
    "  fun_console_write_line(\"'\");\n"
    "  fun_console_write_line(\"\");\n"
    "  return cli_show_help();\n"
    "}\n";

// Helper function to write file (same as cmd_init.c)
static ErrorResult write_file(const char *path, const char *content) {
  StringLength len = fun_string_length(content);

  MemoryResult mem_result = fun_memory_allocate(len);
  if (fun_error_is_error(mem_result.error)) {
    return mem_result.error;
  }

  fun_string_copy(content, (char *)mem_result.value);

  AsyncResult write_result =
      fun_write_memory_to_file((Write){.file_path = (String)path,
                                       .input = mem_result.value,
                                       .bytes_to_write = len});

  fun_console_write_line("  Calling fun_async_await...\n");
  fun_async_await(&write_result);
  fun_console_write_line("  ✓ fun_async_await returned\n");

  voidResult free_result = fun_memory_free(&mem_result.value);
  (void)free_result;

  if (write_result.status != ASYNC_COMPLETED) {
    return fun_error_result(1, "Failed to write file");
  }

  return ERROR_RESULT_NO_ERROR;
}

// Forward declaration - our test is the entry point
int cli_main(void);

int cli_main(void) {
  fun_console_write_line(
      "Testing async file write with LARGE templates...\n\n");

  // Test 1: Write small file (should work)
  fun_console_write_line("Test 1: Small string (35 bytes)\n");
  ErrorResult err =
      write_file("test_small.txt", "Hello, World!\nThis is a test.\n");
  if (fun_error_is_error(err)) {
    fun_console_error_line("FAILED: Small file write\n");
    return 1;
  }
  fun_console_write_line("✓ PASSED: Small file written\n\n");

  // Test 2: Write medium file (should work)
  fun_console_write_line("Test 2: Medium string (~500 bytes)\n");
  err = write_file("test_medium.txt", LARGE_TEMPLATE_1);
  if (fun_error_is_error(err)) {
    fun_console_error_line("FAILED: Medium file write\n");
    return 1;
  }
  fun_console_write_line("✓ PASSED: Medium file written\n\n");

  // Test 3: Write large file (might hang - this is the bug)
  fun_console_write_line(
      "Test 3: Large string (~1500 bytes) - REPRODUCES BUG\n");
  err = write_file("test_large.txt", LARGE_TEMPLATE_3);
  if (fun_error_is_error(err)) {
    fun_console_error_line("FAILED: Large file write (BUG REPRODUCED!)\n");
    return 1;
  }
  fun_console_write_line("✓ PASSED: Large file written\n\n");

  // Test 4: Write multiple files in sequence
  fun_console_write_line("Test 4: Multiple files in sequence\n");
  err = write_file("test_1.txt", LARGE_TEMPLATE_1);
  if (fun_error_is_error(err)) {
    fun_console_error_line("FAILED: First file in sequence\n");
    return 1;
  }
  fun_console_write_line("  ✓ File 1 written\n");

  err = write_file("test_2.txt", LARGE_TEMPLATE_2);
  if (fun_error_is_error(err)) {
    fun_console_error_line("FAILED: Second file in sequence\n");
    return 1;
  }
  fun_console_write_line("  ✓ File 2 written\n");

  err = write_file("test_3.txt", LARGE_TEMPLATE_3);
  if (fun_error_is_error(err)) {
    fun_console_error_line("FAILED: Third file in sequence\n");
    return 1;
  }
  fun_console_write_line("  ✓ File 3 written\n\n");

  fun_console_write_line("=================================\n");
  fun_console_write_line("All tests PASSED!\n");
  fun_console_write_line("=================================\n");

  return 0;
}
