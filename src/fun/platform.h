#ifndef PLATFORM_H
#define PLATFORM_H

#include "vendor/fundamental/include/platform/platform.h"

/**
 * Get the current platform (thin wrapper for convenience)
 */
static inline Platform platform_get(void)
{
	return fun_platform_get(NULL).value;
}

/**
 * Get full platform string into a static buffer.
 * Returns "os-arch" (e.g., "windows-amd64").
 * Not thread-safe (static buffer).
 */
static inline String platform_to_string(Platform platform)
{
	static char buf[32];
	fun_platform_to_string(platform, buf);
	return (String)buf;
}

/**
 * Get the build script filename for the current platform.
 * e.g., "build-windows-amd64.bat" or "build-linux-amd64.sh"
 */
String platform_get_build_script(void);

#endif // PLATFORM_H
