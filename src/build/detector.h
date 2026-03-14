#ifndef BUILD_DETECTOR_H
#define BUILD_DETECTOR_H

#include "vendor/fundamental/include/string/string.h"
#include "vendor/fundamental/include/error/error.h"

/**
 * Build script detection module
 * Checks for existence of platform-specific build scripts
 */

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
 * @param platform_str Platform string (e.g., "windows-amd64")
 * @return BuildDetectionResult with status and script path
 */
BuildDetectionResult build_detect_for_platform(String platform_str);

/**
 * Check for build script existence for current platform
 * @return BuildDetectionResult with status and script path
 */
BuildDetectionResult build_detect_current(void);

/**
 * Check if a specific build script file exists
 * @param script_path Path to the build script
 * @return true if exists, false otherwise
 */
int build_script_exists(String script_path);

#endif // BUILD_DETECTOR_H
