#ifndef BUILD_GENERATOR_H
#define BUILD_GENERATOR_H

#include "vendor/fundamental/include/string/string.h"
#include "vendor/fundamental/include/error/error.h"

/**
 * Build script generation module
 * Scans source files and generates platform-specific build scripts
 */

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
 * Recursively scan src/** / *.c for source files
 * @return SourceScanResult with list of source files
 */
SourceScanResult build_scan_sources(void);

/**
 * Generate Windows build script (build-windows-amd64.bat)
 * @param sources Array of source file paths
 * @param count Number of source files
 * @return BuildGenerationResult with status
 */
BuildGenerationResult build_generate_windows(SourceScanResult scan_result);

/**
 * Generate Linux build script (build-linux-amd64.sh)
 * @param sources Array of source file paths
 * @param count Number of source files
 * @return BuildGenerationResult with status
 */
BuildGenerationResult build_generate_linux(SourceScanResult scan_result);

/**
 * Generate build script for current platform
 * @return BuildGenerationResult with status
 */
BuildGenerationResult build_generate_current(void);

#endif // BUILD_GENERATOR_H
