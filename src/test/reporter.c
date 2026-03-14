#include "reporter.h"
#include "vendor/fundamental/include/console/console.h"

int test_report(TestRunnerResult *result, int verbose)
{
	(void)verbose;
	size_t count = fun_array_TestResult_size(&result->results);
	for (size_t i = 0; i < count; i++) {
		TestResult test = fun_array_TestResult_get(&result->results, i);
		test_report_single(&test);
	}

	test_report_summary(result->passed, result->failed,
						result->passed + result->failed);

	return result->failed > 0 ? 1 : 0;
}

void test_report_single(TestResult *test_result)
{
	if (test_result->status == TEST_RUN_SUCCESS) {
		// Green checkmark
		fun_console_write("\x1b[32m✓\x1b[0m ");
		fun_console_write_line(test_result->name);
	} else {
		// Red X
		fun_console_write("\x1b[31m✗\x1b[0m ");
		fun_console_write(test_result->name);
		fun_console_write(" (exit code: ");
		char code_str[16];
		fun_string_from_int(test_result->exit_code, 10, code_str);
		fun_console_write(code_str);
		fun_console_write_line(")");
	}
}

void test_report_summary(int passed, int failed, int total)
{
	fun_console_write_line("");
	fun_console_write("Summary: ");

	if (passed > 0) {
		fun_console_write("\x1b[32m");
		char passed_str[16];
		fun_string_from_int(passed, 10, passed_str);
		fun_console_write(passed_str);
		fun_console_write(" passed\x1b[0m");
	}

	if (failed > 0 && passed > 0) {
		fun_console_write(", ");
	}

	if (failed > 0) {
		fun_console_write("\x1b[31m");
		char failed_str[16];
		fun_string_from_int(failed, 10, failed_str);
		fun_console_write(failed_str);
		fun_console_write(" failed\x1b[0m");
	}

	fun_console_write(" / ");
	char total_str[16];
	fun_string_from_int(total, 10, total_str);
	fun_console_write(total_str);
	fun_console_write_line(" total");
}
