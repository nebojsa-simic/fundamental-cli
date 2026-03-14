#ifndef TEST_REPORTER_H
#define TEST_REPORTER_H

#include "vendor/fundamental/include/string/string.h"
#include "vendor/fundamental/include/error/error.h"
#include "runner.h"

/**
 * Test reporter module
 * Reports test results with colored output
 */

/**
 * Report test results
 * @param result Test runner result
 * @param verbose Show detailed output
 * @return Exit code (0 if all passed, 1 if any failed)
 */
int test_report(TestRunnerResult *result, int verbose);

/**
 * Report single test result
 * @param test_result Single test result
 */
void test_report_single(TestResult *test_result);

/**
 * Report summary
 * @param passed Number of passed tests
 * @param failed Number of failed tests
 * @param total Total number of tests
 */
void test_report_summary(int passed, int failed, int total);

#endif // TEST_REPORTER_H
