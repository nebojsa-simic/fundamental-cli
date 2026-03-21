#include "build/build.h"
#include "fundamental/filesystem/filesystem.h"
#include "fundamental/console/console.h"
#include "fundamental/string/string.h"

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
 * Check for build script existence for the given platform
 */
BuildDetectionResult build_detect_for_platform(String platform_str)
{
	BuildDetectionResult result;
	static char script_path_buffer[128];

	// Determine file extension based on platform
	const char *ext;
	if (fun_string_index_of(platform_str, (String) "windows", 0) == 0) {
		ext = ".bat";
	} else {
		ext = ".sh";
	}

	// Build script path: build-{platform}.{ext}
	StringTemplateParam params[] = {
		{ .key = (String) "platform", .value = { .stringValue = platform_str } },
		{ .key = (String) "ext", .value = { .stringValue = (String)ext } },
	};
	voidResult tmpl = fun_string_template((String) "build-{platform}{ext}",
										  params, 2, script_path_buffer,
										  sizeof(script_path_buffer));
	if (fun_error_is_error(tmpl.error)) {
		result.status = BUILD_DETECTED_ERROR;
		result.script_path = (String) "";
		result.error_message = (String) "Platform string too long";
		return result;
	}

	result.script_path = (String)script_path_buffer;

	// Check if the file exists
	if (build_script_exists(result.script_path)) {
		result.status = BUILD_DETECTED_EXISTS;
		result.error_message = (String) "";
	} else {
		result.status = BUILD_DETECTED_MISSING;
		result.error_message = (String) "";
	}

	return result;
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
