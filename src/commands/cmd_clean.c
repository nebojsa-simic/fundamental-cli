#include "cmd_clean.h"
#include "build/build.h"
#include "vendor/fundamental/include/async/async.h"
#include "vendor/fundamental/include/console/console.h"
#include "vendor/fundamental/include/process/process.h"

/**
 * Remove the build/ directory and all its contents
 */
static void remove_build_directory(void)
{
	Platform platform = build_platform_get();
	char out_buf[256], err_buf[256];
	ProcessResult proc = { .stdout_data = out_buf,
						   .stdout_capacity = sizeof(out_buf),
						   .stderr_data = err_buf,
						   .stderr_capacity = sizeof(err_buf) };
	AsyncResult spawn_result;

	if (platform.os == PLATFORM_OS_WINDOWS) {
		const char *args[] = { "cmd.exe", "/c",	   "rmdir", "/s",
							   "/q",	  "build", NULL };
		spawn_result = fun_process_spawn("cmd.exe", args, NULL, &proc);
	} else {
		const char *args[] = { "rm", "-rf", "build", NULL };
		spawn_result = fun_process_spawn("rm", args, NULL, &proc);
	}

	fun_async_await(&spawn_result, -1);
	fun_process_free(&proc);
}

/**
 * Execute the clean command
 */
int cmd_clean_execute(int argc, const char **argv)
{
	(void)argc;
	(void)argv;

	fun_console_write_line("Cleaning build artifacts...");

	// Remove build/ directory (contains platform-named binary and any object files)
	remove_build_directory();
	fun_console_write_line("Removed: build/");

	// Clean generated build scripts (optional - user may want to keep them)
	// For now, we don't remove build scripts

	fun_console_write_line("");
	fun_console_write_line("Clean complete!");

	return 0;
}
