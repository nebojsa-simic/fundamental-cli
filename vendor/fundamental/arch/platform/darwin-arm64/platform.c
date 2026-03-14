#include "platform/platform.h"

PlatformOS fun_platform_os(void)
{
	return PLATFORM_OS_DARWIN;
}

PlatformArch fun_platform_arch(void)
{
	return PLATFORM_ARCH_ARM64;
}
