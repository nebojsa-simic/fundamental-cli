#ifndef BUILD_EXECUTOR_H
#define BUILD_EXECUTOR_H

#include "vendor/fundamental/include/string/string.h"
#include "vendor/fundamental/include/error/error.h"

/**
 * Build script execution module
 * Executes platform-specific build scripts and captures results
 */

/**
 * Build execution status
 */
typedef enum {
	BUILD_EXEC_SUCCESS,
	BUILD_EXEC_FAILED,
	BUILD_EXEC_ERROR
} BuildExecutionStatus;

/**
 * Build execution result
 */
typedef struct {
	BuildExecutionStatus status;
	int exit_code;
	String error_message;
} BuildExecutionResult;

/**
 * Execute build script for the given platform
 * @param script_path Path to the build script
 * @param verbose Show detailed output if true
 * @return BuildExecutionResult with status and exit code
 */
BuildExecutionResult build_execute_script(String script_path, int verbose);

/**
 * Execute build script for current platform
 * @param verbose Show detailed output if true
 * @return BuildExecutionResult with status and exit code
 */
BuildExecutionResult build_execute_current(int verbose);

/**
 * Execute Windows batch script
 * @param script_path Path to .bat file
 * @param verbose Show detailed output if true
 * @return BuildExecutionResult with status and exit code
 */
BuildExecutionResult build_execute_windows(String script_path, int verbose);

/**
 * Execute Linux shell script
 * @param script_path Path to .sh file
 * @param verbose Show detailed output if true
 * @return BuildExecutionResult with status and exit code
 */
BuildExecutionResult build_execute_linux(String script_path, int verbose);

#endif // BUILD_EXECUTOR_H
