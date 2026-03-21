#include "cmd_test_add.h"
#include "test/test.h"
#include "fundamental/console/console.h"
#include "fundamental/string/string.h"
#include "fundamental/filesystem/filesystem.h"
#include "fundamental/file/file.h"
#include "fundamental/memory/memory.h"
#include "fundamental/async/async.h"

int cmd_test_add_execute(int argc, const char **argv)
{
	if (argc < 2) {
		fun_console_write_line("Usage: fun test add <module>");
		fun_console_write_line("Example: fun test add string");
		return 1;
	}

	String module_name = (String)argv[1];

	// Create tests/<module>/ directory
	char test_dir[256];
	StringLength module_len = fun_string_length(module_name);

	if (6 + module_len >= sizeof(test_dir)) {
		fun_console_error_line("Error: Module name too long");
		return 1;
	}

	fun_string_copy((String) "tests/", test_dir, sizeof(test_dir));
	fun_string_copy(module_name, test_dir + 6, sizeof(test_dir) - 6);
	test_dir[6 + module_len] = '\0';

	Path _td_path = { (const char *[8]){0}, 0, false };
	fun_path_from_string(test_dir, &_td_path);
	ErrorResult mkdir_result = fun_filesystem_create_directory(_td_path);
	if (fun_error_is_error(mkdir_result)) {
		fun_console_write("Error: Failed to create directory ");
		fun_console_write_line(test_dir);
		return 1;
	}

	// Build test.c path
	char test_c_path[520];
	StringLength test_dir_len = fun_string_length(test_dir);
	fun_string_copy(test_dir, test_c_path, sizeof(test_c_path));
	test_c_path[test_dir_len] = '/';
	fun_string_copy((String) "test.c", test_c_path + test_dir_len + 1,
					sizeof(test_c_path) - test_dir_len - 1);
	test_c_path[test_dir_len + 1 + 6] = '\0';

	// Build script paths
	char win_script_path[520];
	char lin_script_path[520];
	fun_string_copy(test_dir, win_script_path, sizeof(win_script_path));
	win_script_path[test_dir_len] = '/';
	fun_string_copy((String) "build-windows-amd64.bat",
					win_script_path + test_dir_len + 1,
					sizeof(win_script_path) - test_dir_len - 1);
	win_script_path[test_dir_len + 1 + 23] = '\0';

	fun_string_copy(test_dir, lin_script_path, sizeof(lin_script_path));
	lin_script_path[test_dir_len] = '/';
	fun_string_copy((String) "build-linux-amd64.sh",
					lin_script_path + test_dir_len + 1,
					sizeof(lin_script_path) - test_dir_len - 1);
	lin_script_path[test_dir_len + 1 + 20] = '\0';

	// Write test.c file
	char test_content[512];
	StringTemplateParam params[] = { { "module",
									   { .stringValue = module_name } } };
	const char *test_template =
		"#include \"fundamental/console/console.h\"\n"
		"\n"
		"int main(void)\n"
		"{\n"
		"	fun_console_write_line(\"Testing: ${module}\");\n"
		"	\n"
		"	// TODO: Add test assertions\n"
		"	// Return 0 for pass, non-zero for fail\n"
		"	\n"
		"	return 0;\n"
		"}\n";
	fun_string_template(test_template, params, 1, test_content,
						sizeof(test_content));

	MemoryResult mem_result = fun_memory_allocate(512);
	if (fun_error_is_error(mem_result.error)) {
		fun_console_error_line("Error: Failed to allocate memory");
		return 1;
	}
	fun_string_copy(test_content, mem_result.value, 512);

	AsyncResult write_result = fun_write_memory_to_file(
		(Write){ .file_path = test_c_path,
				 .input = mem_result.value,
				 .bytes_to_write = (uint64_t)fun_string_length(test_content),
				 .offset = 0 });
	fun_async_await(&write_result, -1);
	fun_memory_free(&mem_result.value);

	if (write_result.status == ASYNC_ERROR) {
		fun_console_error_line("Failed to write test.c");
		return 1;
	}

	// Generate build scripts with default sources
	const char *sources[] = {
		"../../vendor/fundamental/src/string/stringConversion.c",
		"../../vendor/fundamental/src/string/stringOperations.c",
		"../../vendor/fundamental/src/string/stringTemplate.c",
		"../../vendor/fundamental/src/string/stringValidation.c"
	};

	ScaffoldResult scaffold_result =
		test_scaffold_build_scripts(test_dir, module_name, sources, 4);

	if (scaffold_result.status == SCAFFOLD_ERROR) {
		fun_console_write("Error: ");
		fun_console_write_line(scaffold_result.error_message);
		return 1;
	}

	fun_console_write("Created: ");
	fun_console_write_line(test_dir);
	fun_console_write_line("  - test.c");
	if (scaffold_result.status == SCAFFOLD_SUCCESS) {
		fun_console_write_line("  - build-windows-amd64.bat (generated)");
		fun_console_write_line("  - build-linux-amd64.sh (generated)");
	} else {
		fun_console_write_line("  - build scripts (already exist)");
	}
	fun_console_write_line("");
	fun_console_write("Next: Edit ");
	fun_console_write(test_dir);
	fun_console_write_line("/test.c and run 'fun test'");

	return 0;
}
