#include "build/build.h"
#include "fundamental/async/async.h"
#include "fundamental/console/console.h"
#include "fundamental/filesystem/filesystem.h"
#include "fundamental/process/process.h"
#include "fundamental/string/string.h"

BuildExecutionResult build_execute_for_platform(String script_path, int verbose)
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

	const char *args[] = { "/bin/bash", script_path, NULL };
	char out_buf[4096], err_buf[4096];
	ProcessResult proc = { .stdout_data = out_buf,
						   .stdout_capacity = sizeof(out_buf),
						   .stderr_data = err_buf,
						   .stderr_capacity = sizeof(err_buf) };
	AsyncResult spawn_result = fun_process_spawn("/bin/bash", args, NULL, &proc);
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
