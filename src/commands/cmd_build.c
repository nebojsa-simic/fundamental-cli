#include "cmd_build.h"
#include "cmd_clean.h"
#include "cli/cli.h"
#include "build/build.h"
#include "vendor/fundamental/include/memory/memory.h"
#include "vendor/fundamental/include/console/console.h"
#include "vendor/fundamental/include/string/string.h"

/**
 * Check if a flag is present in arguments
 */
static int has_flag(int argc, const char **argv, const char *flag)
{
	for (int i = 0; i < argc; i++) {
		if (fun_string_compare((String)argv[i], (String)flag) == 0) {
			return 1;
		}
		// Also check long form
		if (flag[0] != '-' && argv[i][0] == '-' && argv[i][1] == '-') {
			if (fun_string_compare((String)(argv[i] + 2), (String)flag) == 0) {
				return 1;
			}
		}
	}
	return 0;
}

/**
 * Execute the build command
 */
int cmd_build_execute(int argc, const char **argv)
{
	int verbose = has_flag(argc, argv, "verbose") || has_flag(argc, argv, "v");
	int release = has_flag(argc, argv, "release") || has_flag(argc, argv, "r");
	int clean = has_flag(argc, argv, "clean");

	if (verbose) {
		fun_console_write_line("fun build");
		fun_console_write_line("Flags:");
		if (verbose)
			fun_console_write_line("  --verbose: enabled");
		if (release)
			fun_console_write_line("  --release: enabled");
		if (clean)
			fun_console_write_line("  --clean: enabled");
		fun_console_write_line("");
	}

	// If --clean flag is set, run clean first
	if (clean) {
		if (verbose) {
			fun_console_write_line("Running clean before build...");
		}
		int clean_result = cmd_clean_execute(0, NULL);
		if (clean_result != 0 && verbose) {
			fun_console_write_line("Warning: clean returned non-zero");
		}
	}

	// Detect platform
	Platform platform = build_platform_get();
	String platform_str = build_platform_to_string(platform);

	if (verbose) {
		fun_console_write("Detected platform: ");
		fun_console_write_line(platform_str);
	}

	// Check if platform is supported
	if (platform.os == PLATFORM_OS_UNKNOWN) {
		fun_console_error_line("Error: Unsupported platform");
		return 1;
	}

	if (platform.arch == PLATFORM_ARCH_UNKNOWN) {
		fun_console_error_line("Warning: Unknown architecture, assuming amd64");
	}

	// Always regenerate the build script so new/removed source files are picked up
	if (verbose) {
		fun_console_write_line("Generating build script...");
	}

	BuildGenerationResult gen_result;
	BuildExecutionResult exec_result;

	gen_result = build_generate_current();

	if (gen_result.status != BUILD_GENERATED_SUCCESS) {
		fun_console_write("Error generating build script: ");
		fun_console_write_line(gen_result.error_message);
		return 1;
	}

	if (verbose) {
		fun_console_write("Generated: ");
		fun_console_write_line(gen_result.script_path);
	}

	// Execute the build script
	if (verbose) {
		fun_console_write_line("Executing build script...");
	}

	exec_result = build_execute_current(verbose);

	if (exec_result.status == BUILD_EXEC_SUCCESS) {
		if (verbose) {
			fun_console_write_line("Build completed successfully!");
		}
		return 0;
	} else if (exec_result.status == BUILD_EXEC_FAILED) {
		fun_console_write("Build failed with exit code: ");
		MemoryResult exit_str_result =
			fun_memory_allocate(16); // Enough for int64_t in base 10
		if (fun_error_is_error(exit_str_result.error)) {
			fun_console_write_line(
				"Error allocating memory for exit code string");
			return 1;
		}
		String exit_str = (String)exit_str_result.value;
		fun_string_from_int(exec_result.exit_code, 10, (OutputString)exit_str,
							16);
		fun_console_write_line(exit_str);
		fun_memory_free((Memory *)exit_str);
		return exec_result.exit_code;
	} else {
		fun_console_write("Build error: ");
		fun_console_write_line(exec_result.error_message);
		return 1;
	}
}
