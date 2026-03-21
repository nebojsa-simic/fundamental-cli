#include "test/test.h"
#include "fundamental/console/console.h"
#include "fundamental/file/file.h"
#include "fundamental/filesystem/filesystem.h"
#include "fundamental/memory/memory.h"
#include "fundamental/async/async.h"

ScaffoldResult test_scaffold_build_scripts(String test_dir, String module_name,
										   const char **sources,
										   int source_count)
{
	ScaffoldResult result;
	result.windows_script = (String) "";
	result.linux_script = (String) "";
	result.error_message = (String) "";

	// Build full script paths
	char win_script_path[520];
	char lin_script_path[520];
	StringLength len = fun_string_length(test_dir);

	if (len + 25 >= sizeof(win_script_path)) {
		result.status = SCAFFOLD_ERROR;
		result.error_message = (String) "Path too long";
		return result;
	}

	fun_string_copy(test_dir, win_script_path, sizeof(win_script_path));
	win_script_path[len] = '/';
	fun_string_copy((String) "build-windows-amd64.bat",
					win_script_path + len + 1,
					sizeof(win_script_path) - len - 1);
	win_script_path[len + 1 + 23] = '\0';

	fun_string_copy(test_dir, lin_script_path, sizeof(lin_script_path));
	lin_script_path[len] = '/';
	fun_string_copy((String) "build-linux-amd64.sh", lin_script_path + len + 1,
					sizeof(lin_script_path) - len - 1);
	lin_script_path[len + 1 + 20] = '\0';

	if (test_has_build_scripts(test_dir)) {
		result.status = SCAFFOLD_ALREADY_EXISTS;
		return result;
	}

	(void)module_name;
	(void)sources;
	(void)source_count;

	// Write Windows script using simple approach
	MemoryResult win_mem = fun_memory_allocate(4096);
	if (fun_error_is_error(win_mem.error)) {
		result.status = SCAFFOLD_ERROR;
		result.error_message = (String) "Failed to allocate memory";
		return result;
	}

	const char *win_content =
		"@ECHO OFF\r\n"
		"REM Build script for test\r\n"
		"\r\n"
		"set FUNDAMENTAL_ROOT=..\\..\\vendor\\fundamental\r\n"
		"\r\n"
		"gcc ^\r\n"
		"    --std=c17 -Os ^\r\n"
		"    -nostdlib ^\r\n"
		"    -fno-builtin ^\r\n"
		"    -fno-exceptions ^\r\n"
		"    -fno-unwind-tables ^\r\n"
		"    -mno-stack-arg-probe ^\r\n"
		"    -e main ^\r\n"
		"    -mconsole ^\r\n"
		"    -I ..\\..\\vendor\\fundamental\\include ^\r\n"
		"    ..\\..\\vendor\\fundamental\\src\\startup\\startup.c ^\r\n"
		"    test.c ^\r\n"
		"    ..\\..\\vendor\\fundamental\\arch\\memory\\windows-amd64\\memory.c ^\r\n"
		"    ..\\..\\vendor\\fundamental\\arch\\console\\windows-amd64\\console.c ^\r\n"
		"    ..\\..\\vendor\\fundamental\\src\\console\\console.c ^\r\n"
		"    ..\\..\\vendor\\fundamental\\src\\string\\stringConversion.c ^\r\n"
		"    ..\\..\\vendor\\fundamental\\src\\string\\stringOperations.c ^\r\n"
		"    ..\\..\\vendor\\fundamental\\src\\string\\stringTemplate.c ^\r\n"
		"    ..\\..\\vendor\\fundamental\\src\\string\\stringValidation.c ^\r\n"
		"    -lkernel32 ^\r\n"
		"    -o test.exe\r\n"
		"\r\n"
		"echo Build complete: test.exe\r\n";

	fun_string_copy(win_content, win_mem.value, 4096);

	AsyncResult win_result = fun_write_memory_to_file(
		(Write){ .file_path = win_script_path,
				 .input = win_mem.value,
				 .bytes_to_write = (uint64_t)fun_string_length(win_content),
				 .offset = 0 });
	fun_async_await(&win_result, -1);
	fun_memory_free(&win_mem.value);

	if (win_result.status == ASYNC_ERROR) {
		result.status = SCAFFOLD_ERROR;
		result.error_message = (String) "Failed to write Windows script";
		return result;
	}

	// Write Linux script
	MemoryResult lin_mem = fun_memory_allocate(4096);
	if (fun_error_is_error(lin_mem.error)) {
		result.status = SCAFFOLD_ERROR;
		result.error_message = (String) "Failed to allocate memory";
		return result;
	}

	const char *lin_content =
		"#!/bin/bash\n"
		"# Build script for test\n"
		"\n"
		"FUNDAMENTAL_ROOT=\"../../vendor/fundamental\"\n"
		"\n"
		"gcc -o test \\\n"
		"    --std=c17 -Os \\\n"
		"    -nostdlib \\\n"
		"    -fno-builtin \\\n"
		"    -fno-exceptions \\\n"
		"    -fno-unwind-tables \\\n"
		"    -e main \\\n"
		"    -I ../../vendor/fundamental/include \\\n"
		"    ../../vendor/fundamental/src/startup/startup.c \\\n"
		"    test.c \\\n"
		"    ../../vendor/fundamental/arch/memory/linux-amd64/memory.c \\\n"
		"    ../../vendor/fundamental/arch/console/linux-amd64/console.c \\\n"
		"    ../../vendor/fundamental/src/console/console.c \\\n"
		"    ../../vendor/fundamental/src/string/stringConversion.c \\\n"
		"    ../../vendor/fundamental/src/string/stringOperations.c \\\n"
		"    ../../vendor/fundamental/src/string/stringTemplate.c \\\n"
		"    ../../vendor/fundamental/src/string/stringValidation.c \\\n"
		"    -lc\n"
		"\n"
		"echo \"Build complete: test\"\n";

	fun_string_copy(lin_content, lin_mem.value, 4096);

	AsyncResult lin_result = fun_write_memory_to_file(
		(Write){ .file_path = lin_script_path,
				 .input = lin_mem.value,
				 .bytes_to_write = (uint64_t)fun_string_length(lin_content),
				 .offset = 0 });
	fun_async_await(&lin_result, -1);
	fun_memory_free(&lin_mem.value);

	if (lin_result.status == ASYNC_ERROR) {
		result.status = SCAFFOLD_ERROR;
		result.error_message = (String) "Failed to write Linux script";
		return result;
	}

	result.status = SCAFFOLD_SUCCESS;
	result.windows_script = win_script_path;
	result.linux_script = lin_script_path;

	return result;
}

int test_has_build_scripts(String test_dir)
{
	char windows_path[520];
	char linux_path[520];
	StringLength len = fun_string_length(test_dir);

	if (len + 25 >= sizeof(windows_path)) {
		return 0;
	}

	fun_string_copy(test_dir, windows_path, sizeof(windows_path));
	windows_path[len] = '/';
	fun_string_copy((String) "build-windows-amd64.bat", windows_path + len + 1,
					sizeof(windows_path) - len - 1);
	windows_path[len + 1 + 23] = '\0';

	fun_string_copy(test_dir, linux_path, sizeof(linux_path));
	linux_path[len] = '/';
	fun_string_copy((String) "build-linux-amd64.sh", linux_path + len + 1,
					sizeof(linux_path) - len - 1);
	linux_path[len + 1 + 20] = '\0';

	Path _wp_path = { (const char *[8]){0}, 0, false };
	fun_path_from_string(windows_path, &_wp_path);
	boolResult win_exists = fun_file_exists(_wp_path);

	Path _lp_path = { (const char *[8]){0}, 0, false };
	fun_path_from_string(linux_path, &_lp_path);
	boolResult lin_exists = fun_file_exists(_lp_path);

	return (fun_error_is_ok(win_exists.error) && win_exists.value) &&
		   (fun_error_is_ok(lin_exists.error) && lin_exists.value);
}

void test_generate_windows_script(String module_name, const char **sources,
								  int source_count, char *output,
								  int output_size)
{
	(void)module_name;
	(void)sources;
	(void)source_count;
	(void)output;
	(void)output_size;
}

void test_generate_linux_script(String module_name, const char **sources,
								int source_count, char *output, int output_size)
{
	(void)module_name;
	(void)sources;
	(void)source_count;
	(void)output;
	(void)output_size;
}
