#include "platform.h"
#include "vendor/fundamental/include/string/string.h"

/**
 * Get the build script filename for the current platform.
 * e.g., "build-windows-amd64.bat" or "build-linux-amd64.sh"
 */
String platform_get_build_script(void)
{
	PlatformResult r = fun_platform_get(NULL);
	Platform platform = r.value;
	static char buffer[64];

	char os_buf[16];
	char arch_buf[16];

	fun_platform_os_to_string(platform.os, os_buf);
	fun_platform_arch_to_string(platform.arch, arch_buf);

	StringLength os_len = fun_string_length((String)os_buf);
	StringLength arch_len = fun_string_length((String)arch_buf);

	const char *ext = (platform.os == PLATFORM_OS_WINDOWS) ? ".bat" : ".sh";
	StringLength ext_len = fun_string_length(ext);

	if (6 + os_len + 1 + arch_len + ext_len + 1 <= sizeof(buffer)) {
		fun_string_copy((String) "build-", buffer);
		fun_string_copy((String)os_buf, buffer + 6);
		buffer[6 + os_len] = '-';
		fun_string_copy((String)arch_buf, buffer + 6 + os_len + 1);
		fun_string_copy((String)ext, buffer + 6 + os_len + 1 + arch_len);
		buffer[6 + os_len + 1 + arch_len + ext_len] = '\0';
		return (String)buffer;
	}

	return (platform.os == PLATFORM_OS_WINDOWS) ?
			       (String) "build-windows-amd64.bat" :
			       (String) "build-linux-amd64.sh";
}
