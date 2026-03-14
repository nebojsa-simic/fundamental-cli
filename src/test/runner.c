#include "runner.h"
#include "scaffolder.h"
#include "vendor/fundamental/include/console/console.h"
#include "vendor/fundamental/include/async/async.h"
#include "vendor/fundamental/include/filesystem/filesystem.h"

TestRunnerResult test_run_all(TestModuleArray *modules, int verbose)
{
	TestRunnerResult result;
	result.status = TEST_RUN_SUCCESS;
	result.passed = 0;
	result.failed = 0;

	TestResultArrayResult results_result =
		fun_array_TestResult_create(fun_array_TestModule_size(modules));
	if (fun_error_is_error(results_result.error)) {
		result.status = TEST_RUN_ERROR;
		TestResultArray empty = { 0 };
		result.results = empty;
		return result;
	}
	result.results = results_result.value;

	size_t count = fun_array_TestModule_size(modules);
	for (size_t i = 0; i < count; i++) {
		TestModule module = fun_array_TestModule_get(modules, i);
		TestResult test_result;
		test_result.name = module.name;

		if (!test_has_build_scripts(module.path)) {
			if (verbose) {
				fun_console_write("Scaffolding build scripts for ");
				fun_console_write_line(module.name);
			}
		}

		int build_result = test_build_module(&module, verbose);
		if (build_result != 0) {
			test_result.status = TEST_RUN_BUILD_FAILED;
			test_result.exit_code = build_result;
			test_result.error_message = (String) "Build failed";
			result.failed++;
			fun_array_TestResult_push(&result.results, test_result);
			continue;
		}

		int exec_result = test_execute_module(&module, verbose);
		if (exec_result == 0) {
			test_result.status = TEST_RUN_SUCCESS;
			test_result.exit_code = 0;
			test_result.error_message = (String) "";
			result.passed++;
		} else {
			test_result.status = TEST_RUN_EXEC_FAILED;
			test_result.exit_code = exec_result;
			test_result.error_message = (String) "Test execution failed";
			result.failed++;
		}

		fun_array_TestResult_push(&result.results, test_result);
	}

	if (result.failed > 0) {
		result.status = TEST_RUN_EXEC_FAILED;
	}

	return result;
}

int test_build_module(TestModule *module, int verbose)
{
	if (verbose) {
		fun_console_write("Building test: ");
		fun_console_write_line(module->name);
	}

	char build_script[520];
	StringLength path_len = fun_string_length(module->path);
	if (path_len + 25 >= sizeof(build_script)) { return 1; }

	fun_string_copy(module->path, build_script);
	build_script[path_len] = '/';
	fun_string_copy((String) "build-windows-amd64.bat", build_script + path_len + 1);
	build_script[path_len + 1 + 23] = '\0';

	boolResult exists = fun_file_exists(build_script);
	if (!fun_error_is_ok(exists.error) || !exists.value) {
		if (verbose) fun_console_write_line("Build script not found");
		return 1;
	}

	// Use cmd.exe /c cd /d <dir> && <script>
	const char *args[] = { "cmd.exe", "/c", "cd", "/d", module->path, "&&", "build-windows-amd64.bat", NULL };
	AsyncResult spawn_result = fun_async_process_spawn("cmd.exe", args, NULL);
	fun_async_await(&spawn_result);

	int exit_code = fun_process_get_exit_code(&spawn_result);
	fun_process_free(&spawn_result);

	if (exit_code != 0 && verbose) {
		fun_console_write("Build failed with exit code: ");
		char code_str[16];
		fun_string_from_int(exit_code, 10, code_str);
		fun_console_write_line(code_str);
	}

	return exit_code;
}

int test_execute_module(TestModule *module, int verbose)
{
	if (verbose) {
		fun_console_write("Running test: ");
		fun_console_write_line(module->name);
	}

	char test_exe[520];
	StringLength path_len = fun_string_length(module->path);
	if (path_len + 10 >= sizeof(test_exe)) { return 1; }

	fun_string_copy(module->path, test_exe);
	test_exe[path_len] = '/';
	fun_string_copy((String) "test.exe", test_exe + path_len + 1);
	test_exe[path_len + 1 + 8] = '\0';

	boolResult exists = fun_file_exists(test_exe);
	if (!fun_error_is_ok(exists.error) || !exists.value) {
		if (verbose) fun_console_write_line("Test executable not found");
		return 1;
	}

	// Run from test directory
	const char *args[] = { "cmd.exe", "/c", "cd", "/d", module->path, "&&", "test.exe", NULL };
	AsyncResult spawn_result = fun_async_process_spawn("cmd.exe", args, NULL);
	fun_async_await(&spawn_result);

	int exit_code = fun_process_get_exit_code(&spawn_result);
	fun_process_free(&spawn_result);

	return exit_code;
}

void test_runner_result_free(TestRunnerResult *result)
{
	size_t count = fun_array_TestResult_size(&result->results);
	(void)count;
	fun_array_TestResult_destroy(&result->results);
}
