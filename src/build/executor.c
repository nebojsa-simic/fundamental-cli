#include "build/build.h"
#include "fundamental/console/console.h"
#include "fundamental/string/string.h"

/**
 * Execute build script for the given platform
 */
BuildExecutionResult build_execute_script(String script_path, int verbose)
{
	return build_execute_for_platform(script_path, verbose);
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

	return build_execute_for_platform(script_path, verbose);
}
