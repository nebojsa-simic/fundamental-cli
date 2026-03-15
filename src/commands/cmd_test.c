#include "cmd_test.h"
#include "../test/discovery.h"
#include "../test/runner.h"
#include "../test/reporter.h"
#include "vendor/fundamental/include/console/console.h"
#include "vendor/fundamental/include/string/string.h"
#include <stddef.h>

/**
 * Check if a flag is present in arguments
 */
static int has_flag(int argc, const char **argv, const char *flag)
{
	for (int i = 0; i < argc; i++) {
		if (fun_string_compare((String)argv[i], (String)flag) == 0) {
			return 1;
		}
		// Check long form --flag
		if (argv[i][0] == '-' && argv[i][1] == '-') {
			if (fun_string_compare((String)(argv[i] + 2), (String)flag) == 0) {
				return 1;
			}
		}
		// Check short form -f
		if (argv[i][0] == '-' && argv[i][1] != '-' && argv[i][1] == flag[0]) {
			return 1;
		}
	}
	return 0;
}

/**
 * Get flag value
 */
static const char *get_flag_value(int argc, const char **argv, const char *flag)
{
	for (int i = 0; i < argc - 1; i++) {
		if (fun_string_compare((String)argv[i], (String)flag) == 0) {
			return argv[i + 1];
		}
	}
	return (const char *)0;
}

int cmd_test_execute(int argc, const char **argv)
{
	int verbose = has_flag(argc, argv, "verbose") || has_flag(argc, argv, "v");
	int list = has_flag(argc, argv, "list") || has_flag(argc, argv, "l");
	(void)argv;

	if (verbose) {
		fun_console_write_line("fun test");
		if (list)
			fun_console_write_line("Mode: list only");
		if (verbose)
			fun_console_write_line("Verbose: enabled");
		fun_console_write_line("");
	}

	// Discover tests
	TestDiscoveryResult discover_result = test_discover((String) "./tests");

	if (discover_result.status == TEST_DISCOVERY_ERROR) {
		fun_console_write("Error discovering tests: ");
		fun_console_write_line(discover_result.error_message);
		test_discovery_free(&discover_result);
		return 1;
	}

	if (discover_result.status == TEST_DISCOVERY_NO_TESTS) {
		if (verbose)
			fun_console_write("DEBUG: ");
		fun_console_write_line(discover_result.error_message);
		fun_console_write_line("No tests found");
		test_discovery_free(&discover_result);
		return 0;
	}

	if (verbose) {
		fun_console_write("DEBUG: Found ");
		char buf[16];
		fun_string_from_int(
			(int64_t)fun_array_TestModule_size(&discover_result.modules), 10,
			buf, sizeof(buf));
		fun_console_write(buf);
		fun_console_write_line(" test modules");
	}

	// List mode - just show tests without running
	if (list) {
		fun_console_write_line("Available tests:");
		size_t count = fun_array_TestModule_size(&discover_result.modules);
		for (size_t i = 0; i < count; i++) {
			TestModule module =
				fun_array_TestModule_get(&discover_result.modules, i);
			fun_console_write("  - ");
			fun_console_write_line(module.name);
		}
		test_discovery_free(&discover_result);
		return 0;
	}

	// Run all tests
	TestRunnerResult run_result =
		test_run_all(&discover_result.modules, verbose);

	// Report results
	int exit_code = test_report(&run_result, verbose);

	// Cleanup
	test_discovery_free(&discover_result);
	test_runner_result_free(&run_result);

	return exit_code;
}
