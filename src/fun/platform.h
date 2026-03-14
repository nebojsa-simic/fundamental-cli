#ifndef PLATFORM_H
#define PLATFORM_H

#include "vendor/fundamental/include/string/string.h"

/**
 * Platform detection module
 * Detects OS and architecture, provides platform string formatting
 */

/**
 * Platform OS enumeration
 */
typedef enum {
	PLATFORM_OS_WINDOWS,
	PLATFORM_OS_LINUX,
	PLATFORM_OS_DARWIN,
	PLATFORM_OS_UNKNOWN
} PlatformOS;

/**
 * Platform architecture enumeration
 */
typedef enum { PLATFORM_ARCH_AMD64, PLATFORM_ARCH_UNKNOWN } PlatformArch;

/**
 * Platform information structure
 */
typedef struct {
	PlatformOS os;
	PlatformArch arch;
} Platform;

/**
 * Detect the current platform OS
 * @return PlatformOS indicating the operating system
 */
PlatformOS platform_detect_os(void);

/**
 * Detect the current platform architecture
 * @return PlatformArch indicating the architecture
 */
PlatformArch platform_detect_arch(void);

/**
 * Get the full platform information
 * @return Platform with OS and architecture detected
 */
Platform platform_get(void);

/**
 * Get platform OS as a string
 * @param os PlatformOS to convert
 * @return String name of the OS (windows, linux, darwin, unknown)
 */
String platform_os_to_string(PlatformOS os);

/**
 * Get platform architecture as a string
 * @param arch PlatformArch to convert
 * @return String name of the architecture (amd64, unknown)
 */
String platform_arch_to_string(PlatformArch arch);

/**
 * Get full platform string (e.g., "windows-amd64")
 * @param platform Platform to convert
 * @return String in format "os-arch" (e.g., "windows-amd64")
 */
String platform_to_string(Platform platform);

/**
 * Get the build script filename for current platform
 * @return String with the build script name (e.g., "build-windows-amd64.bat")
 */
String platform_get_build_script(void);

#endif // PLATFORM_H
