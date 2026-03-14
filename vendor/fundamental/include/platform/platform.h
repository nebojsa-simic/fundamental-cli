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

// ------------------------------------------------------------------
// Platform Detection
// ------------------------------------------------------------------

/**
 * Get the current platform (OS + architecture)
 *
 * Values are provided by the arch layer at link time.
 * Cannot fail — simple constant lookup.
 *
 * @param platform REQUIRED - output pointer to fill
 *
 * Example:
 * Platform p;
 * fun_platform_get(&p);
 * if (p.os == PLATFORM_OS_WINDOWS) { ... }
 */
void fun_platform_get(OutputPlatform platform);

// ------------------------------------------------------------------
// Platform String Conversion
// ------------------------------------------------------------------

/**
 * Write OS name into caller-provided buffer
 *
 * Writes "windows", "linux", "darwin", or "unknown".
 * Buffer must be at least 16 bytes.
 *
 * @param os              PlatformOS value
 * @param platformOsResult REQUIRED - caller-allocated output buffer
 *
 * @return ErrorResult with operation status
 *
 * Example:
 * char buf[16];
 * fun_platform_os_to_string(PLATFORM_OS_LINUX, buf);
 * // buf == "linux"
 */
ErrorResult fun_platform_os_to_string(PlatformOS os,
				       OutputString platformOsResult);

/**
 * Write architecture name into caller-provided buffer
 *
 * Writes "amd64", "arm64", or "unknown".
 * Buffer must be at least 16 bytes.
 *
 * @param arch              PlatformArch value
 * @param platformArchResult REQUIRED - caller-allocated output buffer
 *
 * @return ErrorResult with operation status
 *
 * Example:
 * char buf[16];
 * fun_platform_arch_to_string(PLATFORM_ARCH_AMD64, buf);
 * // buf == "amd64"
 */
ErrorResult fun_platform_arch_to_string(PlatformArch arch,
					 OutputString platformArchResult);

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
