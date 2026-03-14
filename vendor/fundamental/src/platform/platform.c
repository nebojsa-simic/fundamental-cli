#include "platform/platform.h"
#include "string/string.h"

CanReturnError(Platform) fun_platform_get(OutputPlatform platform)
{
	PlatformResult result;
	result.error = ERROR_RESULT_NO_ERROR;

#if defined(_WIN32) || defined(_WIN64)
	result.value.os = PLATFORM_OS_WINDOWS;
#elif defined(__linux__)
	result.value.os = PLATFORM_OS_LINUX;
#elif defined(__APPLE__) && defined(__MACH__)
	result.value.os = PLATFORM_OS_DARWIN;
#else
	result.value.os = PLATFORM_OS_UNKNOWN;
#endif

#if defined(_M_X64) || defined(__x86_64__) || defined(__amd64__)
	result.value.arch = PLATFORM_ARCH_AMD64;
#elif defined(_M_ARM64) || defined(__aarch64__)
	result.value.arch = PLATFORM_ARCH_ARM64;
#else
	result.value.arch = PLATFORM_ARCH_UNKNOWN;
#endif

	if (platform != NULL) {
		*platform = result.value;
	}

	return result;
}

String fun_platform_os_to_string(PlatformOS os)
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

String fun_platform_arch_to_string(PlatformArch arch)
{
	switch (arch) {
	case PLATFORM_ARCH_AMD64:
		return (String) "amd64";
	case PLATFORM_ARCH_ARM64:
		return (String) "arm64";
	default:
		return (String) "unknown";
	}
}

CanReturnError(void) fun_platform_to_string(Platform platform, OutputString output)
{
	voidResult result;

	if (output == NULL) {
		result.error = ERROR_RESULT_NULL_POINTER;
		return result;
	}

	String os_str = fun_platform_os_to_string(platform.os);
	String arch_str = fun_platform_arch_to_string(platform.arch);
	StringLength os_len = fun_string_length(os_str);
	StringLength arch_len = fun_string_length(arch_str);

	fun_string_copy(os_str, output);
	output[os_len] = '-';
	fun_string_copy(arch_str, output + os_len + 1);
	output[os_len + 1 + arch_len] = '\0';

	result.error = ERROR_RESULT_NO_ERROR;
	return result;
}
