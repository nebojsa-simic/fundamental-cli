#include "fundamental/platform/platform.h"
#include "fundamental/string/string.h"

// Arch-layer functions - implemented per platform in arch/platform/<platform>/
PlatformOS fun_platform_os(void);
PlatformArch fun_platform_arch(void);

void fun_platform_get(OutputPlatform platform)
{
	platform->os = fun_platform_os();
	platform->arch = fun_platform_arch();
}

ErrorResult fun_platform_os_to_string(PlatformOS os,
									  OutputString platformOsResult,
									  size_t output_size)
{
	if (platformOsResult == NULL) {
		return ERROR_RESULT_NULL_POINTER;
	}

	String value;
	switch (os) {
	case PLATFORM_OS_WINDOWS:
		value = "windows";
		break;
	case PLATFORM_OS_LINUX:
		value = "linux";
		break;
	case PLATFORM_OS_DARWIN:
		value = "darwin";
		break;
	default:
		value = "unknown";
		break;
	}

	voidResult cr = fun_string_copy(value, platformOsResult, output_size);
	return cr.error;
}

ErrorResult fun_platform_arch_to_string(PlatformArch arch,
										OutputString platformArchResult,
										size_t output_size)
{
	if (platformArchResult == NULL) {
		return ERROR_RESULT_NULL_POINTER;
	}

	String value;
	switch (arch) {
	case PLATFORM_ARCH_AMD64:
		value = "amd64";
		break;
	case PLATFORM_ARCH_ARM64:
		value = "arm64";
		break;
	default:
		value = "unknown";
		break;
	}

	voidResult cr = fun_string_copy(value, platformArchResult, output_size);
	return cr.error;
}

CanReturnError(void)
	fun_platform_to_string(Platform platform, OutputString output,
						   size_t output_size)
{
	voidResult result;

	if (output == NULL) {
		result.error = ERROR_RESULT_NULL_POINTER;
		return result;
	}

	char os_buf[16];
	char arch_buf[16];

	ErrorResult os_err =
		fun_platform_os_to_string(platform.os, os_buf, sizeof(os_buf));
	if (fun_error_is_error(os_err)) {
		result.error = os_err;
		return result;
	}

	ErrorResult arch_err =
		fun_platform_arch_to_string(platform.arch, arch_buf, sizeof(arch_buf));
	if (fun_error_is_error(arch_err)) {
		result.error = arch_err;
		return result;
	}

	StringLength os_len = fun_string_length((String)os_buf);
	StringLength arch_len = fun_string_length((String)arch_buf);

	if (os_len + 1 + arch_len + 1 > output_size) {
		result.error = fun_error_result(ERROR_CODE_BUFFER_TOO_SMALL,
										"Output buffer too small");
		return result;
	}

	voidResult cr = fun_string_copy((String)os_buf, output, output_size);
	if (fun_error_is_error(cr.error)) {
		result.error = cr.error;
		return result;
	}
	output[os_len] = '-';
	cr = fun_string_copy((String)arch_buf, output + os_len + 1,
						 output_size - os_len - 1);
	if (fun_error_is_error(cr.error)) {
		result.error = cr.error;
		return result;
	}
	output[os_len + 1 + arch_len] = '\0';

	result.error = ERROR_RESULT_NO_ERROR;
	return result;
}
