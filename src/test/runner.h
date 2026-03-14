#ifndef TEST_RUNNER_H
#define TEST_RUNNER_H

#include "vendor/fundamental/include/string/string.h"
#include "vendor/fundamental/include/error/error.h"
#include "vendor/fundamental/include/array/array.h"
#include "discovery.h"

/**
 * Test runner module
 * Builds and executes test modules
 */

/**
 * Test execution status
 */
typedef enum {
	TEST_RUN_SUCCESS,
	TEST_RUN_BUILD_FAILED,
	TEST_RUN_EXEC_FAILED,
	TEST_RUN_ERROR
} TestRunStatus;

/**
 * Single test result
 */
typedef struct {
	String name;
	TestRunStatus status;
	int exit_code;
	String error_message;
} TestResult;

/**
 * Array type for test results
 */
DEFINE_ARRAY_TYPE(TestResult)

/**
 * Test runner result
 */
typedef struct {
	TestRunStatus status;
	TestResultArray results;
	int passed;
	int failed;
} TestRunnerResult;

/**
 * Run all discovered tests
 * @param modules Discovered test modules
 * @param verbose Show detailed output
 * @return TestRunnerResult with all test results
 */
TestRunnerResult test_run_all(TestModuleArray *modules, int verbose);

/**
 * Build a single test module
 * @param module Test module to build
 * @param verbose Show detailed output
 * @return 0 on success, non-zero on failure
 */
int test_build_module(TestModule *module, int verbose);

/**
 * Execute a single test binary
 * @param module Test module to execute
 * @param verbose Show detailed output
 * @return Exit code from test execution
 */
int test_execute_module(TestModule *module, int verbose);

/**
 * Free test runner result
 * @param result Result to free
 */
void test_runner_result_free(TestRunnerResult *result);

#endif // TEST_RUNNER_H
