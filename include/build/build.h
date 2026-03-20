#ifndef BUILD_H
#define BUILD_H

#include "fundamental/string/string.h"
#include "fundamental/error/error.h"
#include "fundamental/array/array.h"
#include "fundamental/platform/platform.h"

/* ── Platform helpers ──────────────────────────────────────── */

/**
 * Get the current platform (thin wrapper for convenience)
 */
static inline Platform build_platform_get(void)
{
	Platform p;
	fun_platform_get(&p);
	return p;
}

/**
 * Get full platform string into a static buffer.
 * Returns "os-arch" (e.g., "windows-amd64").
 * Not thread-safe (static buffer).
 */
static inline String build_platform_to_string(Platform platform)
{
	static char buf[32];
	fun_platform_to_string(platform, buf, sizeof(buf));
	return (String)buf;
}

/**
 * Get the build script filename for the current platform.
 * e.g., "build-windows-amd64.bat" or "build-linux-amd64.sh"
 */
String build_platform_get_script(void);

/* ── Build configuration ──────────────────────────────────── */

#define MAX_CONFIG_VALUE 256

/**
 * Build configuration structure
 */
typedef struct {
	String entry_point;
	String flags;
	String standard;
	String output;
	int use_nostdlib;
} BuildConfig;

/**
 * Parse build configuration from fun.ini
 * @return BuildConfig with parsed values
 */
BuildConfig build_config_load(void);

/**
 * Get entry point from config
 */
String build_config_get_entry(BuildConfig config);

/**
 * Get custom flags from config
 */
String build_config_get_flags(BuildConfig config);

/* ── Build detection ──────────────────────────────────────── */

/**
 * Build detection status
 */
typedef enum {
	BUILD_DETECTED_EXISTS,
	BUILD_DETECTED_MISSING,
	BUILD_DETECTED_ERROR
} BuildDetectionStatus;

/**
 * Build detection result
 */
typedef struct {
	BuildDetectionStatus status;
	String script_path;
	String error_message;
} BuildDetectionResult;

/**
 * Check for build script existence for the given platform
 */
BuildDetectionResult build_detect_for_platform(String platform_str);

/**
 * Check for build script existence for current platform
 */
BuildDetectionResult build_detect_current(void);

/**
 * Check if a specific build script file exists
 */
int build_script_exists(String script_path);

/* ── Build execution ──────────────────────────────────────── */

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
 */
BuildExecutionResult build_execute_script(String script_path, int verbose);

/**
 * Execute build script for current platform
 */
BuildExecutionResult build_execute_current(int verbose);

/**
 * Execute Windows batch script
 */
BuildExecutionResult build_execute_windows(String script_path, int verbose);

/**
 * Execute Linux shell script
 */
BuildExecutionResult build_execute_linux(String script_path, int verbose);

/* ── Build generation ─────────────────────────────────────── */

#define MAX_SOURCE_FILES 256
#define MAX_SOURCES_LENGTH 4096

/**
 * Source scan result
 */
typedef struct {
	String sources[MAX_SOURCE_FILES];
	size_t count;
	String error_message;
} SourceScanResult;

/**
 * Build generation status
 */
typedef enum {
	BUILD_GENERATED_SUCCESS,
	BUILD_GENERATED_ERROR
} BuildGenerationStatus;

/**
 * Build generation result
 */
typedef struct {
	BuildGenerationStatus status;
	String script_path;
	String error_message;
} BuildGenerationResult;

/**
 * Recursively scan for source files
 */
SourceScanResult build_scan_sources(void);

/**
 * Generate Windows build script
 */
BuildGenerationResult build_generate_windows(SourceScanResult scan_result);

/**
 * Generate Linux build script
 */
BuildGenerationResult build_generate_linux(SourceScanResult scan_result);

/**
 * Generate build script for current platform
 */
BuildGenerationResult build_generate_current(void);

#endif // BUILD_H
