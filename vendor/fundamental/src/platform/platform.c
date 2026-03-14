#include "platform/platform.h"
#include "string/string.h"

// Arch-layer functions - implemented per platform in arch/platform/<platform>/
PlatformOS fun_platform_os(void);
PlatformArch fun_platform_arch(void);

CanReturnError(Platform) fun_platform_get(OutputPlatform platform)
{
	PlatformResult result;
	result.error = ERROR_RESULT_NO_ERROR;
	result.value.os = fun_platform_os();
	result.value.arch = fun_platform_arch();

	if (platform != NULL) {
		*platform = result.value;
	}

	return result;
}

ErrorResult fun_platform_os_to_string(PlatformOS os,
				       OutputString platformOsResult)
{
	if (platformOsResult == NULL) {
		return ERROR_RESULT_NULL_POINTER;
	}

	switch (os) {
	case PLATFORM_OS_WINDOWS:
		fun_string_copy((String) "windows", platformOsResult);
		break;
	case PLATFORM_OS_LINUX:
		fun_string_copy((String) "linux", platformOsResult);
		break;
	case PLATFORM_OS_DARWIN:
		fun_string_copy((String) "darwin", platformOsResult);
		break;
	default:
		fun_string_copy((String) "unknown", platformOsResult);
		break;
	}

	return ERROR_RESULT_NO_ERROR;
}

ErrorResult fun_platform_arch_to_string(PlatformArch arch,
					 OutputString platformArchResult)
{
	if (platformArchResult == NULL) {
		return ERROR_RESULT_NULL_POINTER;
	}

	switch (arch) {
	case PLATFORM_ARCH_AMD64:
		fun_string_copy((String) "amd64", platformArchResult);
		break;
	case PLATFORM_ARCH_ARM64:
		fun_string_copy((String) "arm64", platformArchResult);
		break;
	default:
		fun_string_copy((String) "unknown", platformArchResult);
		break;
	}

	return ERROR_RESULT_NO_ERROR;
}

CanReturnError(void) fun_platform_to_string(Platform platform,
					     OutputString output)
{
	voidResult result;

	if (output == NULL) {
		result.error = ERROR_RESULT_NULL_POINTER;
		return result;
	}

	char os_buf[16];
	char arch_buf[16];

	ErrorResult os_err = fun_platform_os_to_string(platform.os, os_buf);
	if (fun_error_is_error(os_err)) {
		result.error = os_err;
		return result;
	}

	ErrorResult arch_err =
		fun_platform_arch_to_string(platform.arch, arch_buf);
	if (fun_error_is_error(arch_err)) {
		result.error = arch_err;
		return result;
	}

	StringLength os_len = fun_string_length((String)os_buf);
	StringLength arch_len = fun_string_length((String)arch_buf);

	fun_string_copy((String)os_buf, output);
	output[os_len] = '-';
	fun_string_copy((String)arch_buf, output + os_len + 1);
	output[os_len + 1 + arch_len] = '\0';

	result.error = ERROR_RESULT_NO_ERROR;
	return result;
}
