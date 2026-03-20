#include "fundamental/platform/platform.h"

PlatformOS fun_platform_os(void)
{
	return PLATFORM_OS_LINUX;
}

PlatformArch fun_platform_arch(void)
{
	return PLATFORM_ARCH_AMD64;
}
