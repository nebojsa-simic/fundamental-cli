#include "build/build.h"
#include "fundamental/async/async.h"
#include "fundamental/console/console.h"
#include "fundamental/filesystem/filesystem.h"
#include "fundamental/process/process.h"

/**
 * Execute Windows batch script
 */
BuildExecutionResult build_execute_windows(String script_path, int verbose)
{
	BuildExecutionResult result;

	char cwd[512];
	char full_path[1024];
	ErrorResult cwd_result = fun_filesystem_get_working_directory(cwd);
	if (fun_error_is_ok(cwd_result)) {
		char cwd_buf[512];
		const char *cwd_comps[16];
		Path cwd_path = { cwd_comps, 0, false };
		fun_path_from_cstr(cwd, cwd_buf, sizeof(cwd_buf), &cwd_path);

		char sp_buf[256];
		const char *sp_comps[8];
		Path sp_path = { sp_comps, 0, false };
		fun_path_from_cstr(script_path, sp_buf, sizeof(sp_buf), &sp_path);

		const char *joined_comps[24];
		Path joined = { joined_comps, 0, false };
		fun_path_join(cwd_path, sp_path, &joined);
		fun_path_to_string(joined, full_path, sizeof(full_path));
		script_path = full_path;
	}

	if (verbose) {
		fun_console_write_line("Executing Windows build script...");
		fun_console_write("Running: ");
		fun_console_write_line(script_path);
	}

	const char *args[] = { "cmd.exe", "/c", script_path, NULL };
	char out_buf[4096], err_buf[4096];
	ProcessResult proc = { .stdout_data = out_buf,
						   .stdout_capacity = sizeof(out_buf),
						   .stderr_data = err_buf,
						   .stderr_capacity = sizeof(err_buf) };
	AsyncResult spawn_result = fun_process_spawn("cmd.exe", args, NULL, &proc);
	fun_async_await(&spawn_result, -1);

	int exit_code = proc.exit_code;
	fun_process_free(&proc);

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

	char cwd[512];
	char full_path[1024];
	ErrorResult cwd_result = fun_filesystem_get_working_directory(cwd);
	if (fun_error_is_ok(cwd_result)) {
		char cwd_buf[512];
		const char *cwd_comps[16];
		Path cwd_path = { cwd_comps, 0, false };
		fun_path_from_cstr(cwd, cwd_buf, sizeof(cwd_buf), &cwd_path);

		char sp_buf[256];
		const char *sp_comps[8];
		Path sp_path = { sp_comps, 0, false };
		fun_path_from_cstr(script_path, sp_buf, sizeof(sp_buf), &sp_path);

		const char *joined_comps[24];
		Path joined = { joined_comps, 0, false };
		fun_path_join(cwd_path, sp_path, &joined);
		fun_path_to_string(joined, full_path, sizeof(full_path));
		script_path = full_path;
	}

	if (verbose) {
		fun_console_write_line("Executing Linux build script...");
		fun_console_write("Running: ");
		fun_console_write_line(script_path);
	}

	const char *args[] = { "bash", script_path, NULL };
	char out_buf[4096], err_buf[4096];
	ProcessResult proc = { .stdout_data = out_buf,
						   .stdout_capacity = sizeof(out_buf),
						   .stderr_data = err_buf,
						   .stderr_capacity = sizeof(err_buf) };
	AsyncResult spawn_result = fun_process_spawn("bash", args, NULL, &proc);
	fun_async_await(&spawn_result, -1);

	int exit_code = proc.exit_code;
	fun_process_free(&proc);

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
	Platform platform = build_platform_get();
	String script_path = build_platform_get_script();

	if (verbose) {
		fun_console_write("Detected platform: ");
		fun_console_write_line(build_platform_to_string(platform));
	}

	return build_execute_script(script_path, verbose);
}
