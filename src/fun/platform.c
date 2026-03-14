#include "platform.h"
#include "vendor/fundamental/include/console/console.h"

/**
 * Detect the current platform OS using preprocessor macros
 */
PlatformOS platform_detect_os(void)
{
#if defined(_WIN32) || defined(_WIN64)
	return PLATFORM_OS_WINDOWS;
#elif defined(__linux__)
	return PLATFORM_OS_LINUX;
#elif defined(__APPLE__) && defined(__MACH__)
	return PLATFORM_OS_DARWIN;
#else
	return PLATFORM_OS_UNKNOWN;
#endif
}

/**
 * Detect the current platform architecture
 * For now, we only support amd64
 */
PlatformArch platform_detect_arch(void)
{
#if defined(_M_X64) || defined(__x86_64__) || defined(__amd64__)
	return PLATFORM_ARCH_AMD64;
#else
	return PLATFORM_ARCH_UNKNOWN;
#endif
}

/**
 * Get the full platform information
 */
Platform platform_get(void)
{
	Platform platform;
	platform.os = platform_detect_os();
	platform.arch = platform_detect_arch();
	return platform;
}

/**
 * Convert PlatformOS to string
 */
String platform_os_to_string(PlatformOS os)
{
	switch (os) {
	case PLATFORM_OS_WINDOWS:
		return (String) "windows";
	case PLATFORM_OS_LINUX:
		return (String) "linux";
	case PLATFORM_OS_DARWIN:
		return (String) "darwin";
	default:
		return (String) "unknown";
	}
}

/**
 * Convert PlatformArch to string
 */
String platform_arch_to_string(PlatformArch arch)
{
	switch (arch) {
	case PLATFORM_ARCH_AMD64:
		return (String) "amd64";
	default:
		return (String) "unknown";
	}
}

/**
 * Get full platform string (e.g., "windows-amd64")
 * Note: Returns a static string based on current platform
 */
String platform_to_string(Platform platform)
{
	static char buffer[32];
	String os_str = platform_os_to_string(platform.os);
	String arch_str = platform_arch_to_string(platform.arch);

	// Build the platform string
	StringLength os_len = fun_string_length(os_str);
	StringLength arch_len = fun_string_length(arch_str);

	if (os_len + arch_len + 1 < sizeof(buffer)) {
		fun_string_copy(os_str, buffer);
		buffer[os_len] = '-';
		fun_string_copy(arch_str, buffer + os_len + 1);
		buffer[os_len + 1 + arch_len] = '\0';
		return (String)buffer;
	}

	return (String) "unknown-unknown";
}

/**
 * Get the build script filename for current platform
 */
String platform_get_build_script(void)
{
	Platform platform = platform_get();
	static char buffer[64];

	String os_str = platform_os_to_string(platform.os);
	String arch_str = platform_arch_to_string(platform.arch);

	// Build script name format: build-{os}-{arch}.{ext}
	StringLength os_len = fun_string_length(os_str);
	StringLength arch_len = fun_string_length(arch_str);
	const char *ext;

	if (platform.os == PLATFORM_OS_WINDOWS) {
		ext = ".bat";
	} else {
		ext = ".sh";
	}

	StringLength ext_len = fun_string_length(ext);

	if (os_len + arch_len + ext_len + 13 < sizeof(buffer)) {
		fun_string_copy((String) "build-", buffer);
		fun_string_copy(os_str, buffer + 6);
		buffer[6 + os_len] = '-';
		fun_string_copy(arch_str, buffer + 6 + os_len + 1);
		fun_string_copy((String)ext, buffer + 6 + os_len + 1 + arch_len);
		buffer[6 + os_len + 1 + arch_len + ext_len] = '\0';
		return (String)buffer;
	}

	if (platform.os == PLATFORM_OS_WINDOWS) {
		return (String) "build-windows-amd64.bat";
	} else {
		return (String) "build-linux-amd64.sh";
	}
}
