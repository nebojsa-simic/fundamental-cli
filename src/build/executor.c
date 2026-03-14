#include "executor.h"
#include "../fun/platform.h"
#include "vendor/fundamental/include/async/async.h"
#include "vendor/fundamental/include/console/console.h"

/**
 * Execute Windows batch script
 */
BuildExecutionResult build_execute_windows(String script_path, int verbose)
{
	BuildExecutionResult result;

	if (verbose) {
		fun_console_write_line("Executing Windows build script...");
		fun_console_write("Running: ");
		fun_console_write_line(script_path);
	}

	const char *args[] = { "cmd.exe", "/c", script_path, NULL };
	AsyncResult spawn_result = fun_async_process_spawn("cmd.exe", args, NULL);
	fun_async_await(&spawn_result);

	int exit_code = fun_process_get_exit_code(&spawn_result);
	fun_process_free(&spawn_result);

	if (exit_code == 0) {
		result.status = BUILD_EXEC_SUCCESS;
		result.exit_code = 0;
		result.error_message = (String) "";
	} else {
		result.status = BUILD_EXEC_FAILED;
		result.exit_code = exit_code;
		result.error_message = (String) "Build script failed";
	}

	return result;
}

/**
 * Execute Linux shell script
 */
BuildExecutionResult build_execute_linux(String script_path, int verbose)
{
	BuildExecutionResult result;

	if (verbose) {
		fun_console_write_line("Executing Linux build script...");
		fun_console_write("Running: ");
		fun_console_write_line(script_path);
	}

	const char *args[] = { "bash", script_path, NULL };
	AsyncResult spawn_result = fun_async_process_spawn("bash", args, NULL);
	fun_async_await(&spawn_result);

	int exit_code = fun_process_get_exit_code(&spawn_result);
	fun_process_free(&spawn_result);

	if (exit_code == 0) {
		result.status = BUILD_EXEC_SUCCESS;
		result.exit_code = 0;
		result.error_message = (String) "";
	} else {
		result.status = BUILD_EXEC_FAILED;
		result.exit_code = exit_code;
		result.error_message = (String) "Build script failed";
	}

	return result;
}

/**
 * Execute build script for the given platform
 */
BuildExecutionResult build_execute_script(String script_path, int verbose)
{
	// Determine platform from script extension
	StringLength len = fun_string_length(script_path);

	// Check if it's a Windows batch file
	if (len > 4) {
		const char *ext = (const char *)(script_path + len - 4);
		if (ext[0] == '.' && ext[1] == 'b' && ext[2] == 'a' && ext[3] == 't') {
			return build_execute_windows(script_path, verbose);
		}
	}

	// Check if it's a shell script
	if (len > 3) {
		const char *ext = (const char *)(script_path + len - 3);
		if (ext[0] == '.' && ext[1] == 's' && ext[2] == 'h') {
			return build_execute_linux(script_path, verbose);
		}
	}

	// Unknown script type, try Linux execution as default
	return build_execute_linux(script_path, verbose);
}

/**
 * Execute build script for current platform
 */
BuildExecutionResult build_execute_current(int verbose)
{
	Platform platform = platform_get();
	String script_path = platform_get_build_script();

	if (verbose) {
		fun_console_write("Detected platform: ");
		fun_console_write_line(platform_to_string(platform));
	}

	return build_execute_script(script_path, verbose);
}
