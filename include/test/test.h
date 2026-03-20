#ifndef TEST_H
#define TEST_H

#include "fundamental/string/string.h"
#include "fundamental/error/error.h"
#include "fundamental/array/array.h"

/* ── Test discovery ───────────────────────────────────────── */

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
 */
TestDiscoveryResult test_discover(String tests_dir);

/**
 * Check if a directory contains a test.c file
 */
int test_has_test_file(String dir_path);

/**
 * Free test discovery result
 */
void test_discovery_free(TestDiscoveryResult *result);

/* ── Test runner ──────────────────────────────────────────── */

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
 */
TestRunnerResult test_run_all(TestModuleArray *modules, int verbose);

/**
 * Build a single test module
 */
int test_build_module(TestModule *module, int verbose);

/**
 * Execute a single test binary
 */
int test_execute_module(TestModule *module, int verbose);

/**
 * Free test runner result
 */
void test_runner_result_free(TestRunnerResult *result);

/* ── Test reporter ────────────────────────────────────────── */

/**
 * Report test results
 */
int test_report(TestRunnerResult *result, int verbose);

/**
 * Report single test result
 */
void test_report_single(TestResult *test_result);

/**
 * Report summary
 */
void test_report_summary(int passed, int failed, int total);

/* ── Test scaffolder ──────────────────────────────────────── */

/**
 * Scaffold status
 */
typedef enum {
	SCAFFOLD_SUCCESS,
	SCAFFOLD_ALREADY_EXISTS,
	SCAFFOLD_ERROR
} ScaffoldStatus;

/**
 * Scaffold result
 */
typedef struct {
	ScaffoldStatus status;
	String windows_script;
	String linux_script;
	String error_message;
} ScaffoldResult;

/**
 * Generate build scripts for a test module
 */
ScaffoldResult test_scaffold_build_scripts(String test_dir, String module_name,
										   const char **sources,
										   int source_count);

/**
 * Check if build scripts already exist
 */
int test_has_build_scripts(String test_dir);

/**
 * Generate Windows build script content
 */
void test_generate_windows_script(String module_name, const char **sources,
								  int source_count, char *output,
								  int output_size);

/**
 * Generate Linux build script content
 */
void test_generate_linux_script(String module_name, const char **sources,
								int source_count, char *output,
								int output_size);

/* ── Module map ───────────────────────────────────────────── */

/**
 * Maximum sources per module
 */
#define MAX_MODULE_SOURCES 32

/**
 * Module mapping entry
 */
typedef struct {
	String module_name;
	const char *sources[MAX_MODULE_SOURCES];
	int source_count;
} ModuleMapping;

/**
 * Array type for module mappings
 */
DEFINE_ARRAY_TYPE(ModuleMapping)

/**
 * Module mapping lookup result
 */
typedef struct {
	int found;
	const char **sources;
	int source_count;
} ModuleMapLookupResult;

/**
 * Initialize module mappings
 */
ModuleMappingArrayResult test_module_map_init(void);

/**
 * Get sources for a module
 */
ModuleMapLookupResult test_module_map_get(ModuleMappingArray *mappings,
										  String module_name);

/**
 * Free module mappings
 */
void test_module_map_free(ModuleMappingArray *mappings);

#endif // TEST_H
