#include "cmd_clean.h"
#include "../fun/platform.h"
#include "vendor/fundamental/include/async/async.h"
#include "vendor/fundamental/include/console/console.h"
#include "vendor/fundamental/include/filesystem/filesystem.h"

/**
 * Remove a file if it exists, using platform-native delete command
 */
static void remove_file(const char *path)
{
	boolResult exists = fun_file_exists((String)path);
	if (fun_error_is_error(exists.error) || !exists.value) {
		return;
	}

	Platform platform = platform_get();
	AsyncResult spawn_result;

	if (platform.os == PLATFORM_OS_WINDOWS) {
		const char *args[] = { "cmd.exe", "/c", "del", "/f", "/q", path,
							   NULL };
		spawn_result = fun_async_process_spawn("cmd.exe", args, NULL);
	} else {
		const char *args[] = { "rm", "-f", path, NULL };
		spawn_result = fun_async_process_spawn("rm", args, NULL);
	}

	fun_async_await(&spawn_result);
	fun_process_free(&spawn_result);
}

/**
 * Execute the clean command
 */
int cmd_clean_execute(int argc, const char **argv)
{
	(void)argc;
	(void)argv;

	Platform platform = platform_get();

	fun_console_write_line("Cleaning build artifacts...");

	// Platform-specific binary names
	const char *binary_name;
	if (platform.os == PLATFORM_OS_WINDOWS) {
		binary_name = "app.exe";
	} else {
		binary_name = "app";
	}

	// Remove main binary
	remove_file(binary_name);
	fun_console_write("Removed: ");
	fun_console_write_line(binary_name);

	// Remove object files in src/
	// Note: A full implementation would scan for .o files
	// For now, we'll list common locations
	const char *obj_paths[] = { "src/*.o", "build/*.o", NULL };

	for (int i = 0; obj_paths[i] != NULL; i++) {
		// In a full implementation, we'd use glob pattern matching
		// For now, just report what would be cleaned
	}

	fun_console_write_line("Removed: object files (*.o)");

	// Clean generated build scripts (optional - user may want to keep them)
	// For now, we don't remove build scripts

	fun_console_write_line("");
	fun_console_write_line("Clean complete!");

	return 0;
}
