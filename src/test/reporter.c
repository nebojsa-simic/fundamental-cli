#include "test/test.h"
#include "fundamental/console/console.h"

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
		char msg[256];
		StringTemplateParam p[] = {
			{ .key = (String) "name",
			  .value = { .stringValue = test_result->name } },
			{ .key = (String) "code",
			  .value = { .intValue = test_result->exit_code } },
		};
		fun_string_template(
			(String) "\x1b[31m✗\x1b[0m ${name} (exit code: #{code})", p, 2, msg,
			sizeof(msg));
		fun_console_write_line(msg);
	}
}

void test_report_summary(int passed, int failed, int total)
{
	fun_console_write_line("");
	fun_console_write("Summary: ");

	char buf[32];
	StringTemplateParam p[] = { { .key = (String) "n" } };

	if (passed > 0) {
		p[0].value.intValue = passed;
		fun_string_template((String) "\x1b[32m#{n} passed\x1b[0m", p, 1, buf,
							sizeof(buf));
		fun_console_write(buf);
	}

	if (failed > 0 && passed > 0) {
		fun_console_write(", ");
	}

	if (failed > 0) {
		p[0].value.intValue = failed;
		fun_string_template((String) "\x1b[31m#{n} failed\x1b[0m", p, 1, buf,
							sizeof(buf));
		fun_console_write(buf);
	}

	p[0].value.intValue = total;
	fun_string_template((String) " / #{n} total", p, 1, buf, sizeof(buf));
	fun_console_write_line(buf);
}
