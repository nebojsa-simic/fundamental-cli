#ifndef TEST_SCAFFOLDER_H
#define TEST_SCAFFOLDER_H

#include "vendor/fundamental/include/string/string.h"
#include "vendor/fundamental/include/error/error.h"

/**
 * Test build scaffolding module
 * Generates build scripts for test modules
 */

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
 * @param test_dir Path to test directory
 * @param module_name Name of the module being tested
 * @param sources Array of source files to include
 * @param source_count Number of source files
 * @return ScaffoldResult with generated script paths
 */
ScaffoldResult test_scaffold_build_scripts(String test_dir, String module_name,
										   const char **sources,
										   int source_count);

/**
 * Check if build scripts already exist
 * @param test_dir Path to test directory
 * @return 1 if both scripts exist, 0 otherwise
 */
int test_has_build_scripts(String test_dir);

/**
 * Generate Windows build script content
 * @param module_name Name of module
 * @param sources Source files to include
 * @param source_count Number of sources
 * @param output Buffer to write script to
 * @param output_size Size of output buffer
 */
void test_generate_windows_script(String module_name, const char **sources,
								  int source_count, char *output,
								  int output_size);

/**
 * Generate Linux build script content
 * @param module_name Name of module
 * @param sources Source files to include
 * @param source_count Number of sources
 * @param output Buffer to write script to
 * @param output_size Size of output buffer
 */
void test_generate_linux_script(String module_name, const char **sources,
								int source_count, char *output,
								int output_size);

#endif // TEST_SCAFFOLDER_H
