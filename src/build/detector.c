#include "build/build.h"
#include "fundamental/filesystem/filesystem.h"

/**
 * Check if a specific build script file exists
 */
int build_script_exists(String script_path)
{
	char _sp_buf[256];
	Path _sp_path = { (const char *[8]){0}, 0, false };
	fun_path_from_cstr(script_path, _sp_buf, sizeof(_sp_buf), &_sp_path);
	boolResult result = fun_file_exists(_sp_path);
	return fun_error_is_ok(result.error) && result.value;
}

/**
 * Check for build script existence for current platform
 */
BuildDetectionResult build_detect_current(void)
{
	Platform platform = build_platform_get();
	String platform_str = build_platform_to_string(platform);
	return build_detect_for_platform(platform_str);
}
