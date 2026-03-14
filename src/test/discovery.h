#ifndef TEST_DISCOVERY_H
#define TEST_DISCOVERY_H

#include "vendor/fundamental/include/string/string.h"
#include "vendor/fundamental/include/error/error.h"
#include "vendor/fundamental/include/array/array.h"

/**
 * Test discovery module
 * Scans tests/ directory to find test modules
 */

/**
 * Test module info
 */
typedef struct {
	String name;
	String path;
	String test_file;
} TestModule;

/**
 * Array type for test modules
 */
DEFINE_ARRAY_TYPE(TestModule)

/**
 * Test discovery status
 */
typedef enum {
	TEST_DISCOVERY_SUCCESS,
	TEST_DISCOVERY_NO_TESTS,
	TEST_DISCOVERY_ERROR
} TestDiscoveryStatus;

/**
 * Test discovery result
 */
typedef struct {
	TestDiscoveryStatus status;
	TestModuleArray modules;
	String error_message;
} TestDiscoveryResult;

/**
 * Discover all test modules in tests/ directory
 * @param tests_dir Path to tests directory
 * @return TestDiscoveryResult with list of test modules
 */
TestDiscoveryResult test_discover(String tests_dir);

/**
 * Check if a directory contains a test.c file
 * @param dir_path Path to directory
 * @return 1 if test.c exists, 0 otherwise
 */
int test_has_test_file(String dir_path);

/**
 * Free test discovery result
 * @param result Result to free
 */
void test_discovery_free(TestDiscoveryResult *result);

#endif // TEST_DISCOVERY_H
