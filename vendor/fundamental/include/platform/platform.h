#ifndef LIBRARY_PLATFORM_H
#define LIBRARY_PLATFORM_H

#include "../error/error.h"
#include "../string/string.h"

// ------------------------------------------------------------------
// Platform Module Types
// ------------------------------------------------------------------

typedef enum {
	PLATFORM_OS_WINDOWS,
	PLATFORM_OS_LINUX,
	PLATFORM_OS_DARWIN,
	PLATFORM_OS_UNKNOWN
} PlatformOS;

typedef enum {
	PLATFORM_ARCH_AMD64,
	PLATFORM_ARCH_ARM64,
	PLATFORM_ARCH_UNKNOWN
} PlatformArch;

typedef struct {
	PlatformOS os;
	PlatformArch arch;
} Platform;

typedef Platform *OutputPlatform;

DEFINE_RESULT_TYPE(Platform);

// ------------------------------------------------------------------
// Platform Detection
// ------------------------------------------------------------------

/**
 * Get the current platform (OS + architecture)
 *
 * Determined at compile time via preprocessor macros.
 *
 * @param platform OPTIONAL - output pointer to fill, or NULL
 *
 * @return PlatformResult with value populated, error always NO_ERROR
 *
 * Example:
 * PlatformResult r = fun_platform_get(NULL);
 * if (r.value.os == PLATFORM_OS_WINDOWS) { ... }
 *
 * Platform p;
 * fun_platform_get(&p);
 */
CanReturnError(Platform) fun_platform_get(OutputPlatform platform);

// ------------------------------------------------------------------
// Platform String Conversion
// ------------------------------------------------------------------

/**
 * Get OS name as a string literal
 *
 * @param os PlatformOS value
 * @return "windows", "linux", "darwin", or "unknown"
 *
 * Example:
 * String name = fun_platform_os_to_string(PLATFORM_OS_LINUX);
 */
String fun_platform_os_to_string(PlatformOS os);

/**
 * Get architecture name as a string literal
 *
 * @param arch PlatformArch value
 * @return "amd64", "arm64", or "unknown"
 *
 * Example:
 * String name = fun_platform_arch_to_string(PLATFORM_ARCH_AMD64);
 */
String fun_platform_arch_to_string(PlatformArch arch);

/**
 * Write full platform string into caller-provided buffer
 *
 * Format: "<os>-<arch>" (e.g., "windows-amd64", "linux-arm64")
 * Buffer must be at least 32 bytes.
 *
 * @param platform Platform to convert
 * @param output   REQUIRED - caller-allocated output buffer
 *
 * @return ErrorResult with operation status
 *
 * Example:
 * char buf[32];
 * fun_platform_to_string(fun_platform_get(NULL).value, buf);
 * // buf == "windows-amd64"
 */
CanReturnError(void) fun_platform_to_string(Platform platform,
											OutputString output);

#endif // LIBRARY_PLATFORM_H
