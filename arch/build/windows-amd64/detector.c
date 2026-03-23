#include "build/build.h"
#include "fundamental/string/string.h"

BuildDetectionResult build_detect_for_platform(String platform_str)
{
	BuildDetectionResult result;
	static char script_path_buffer[128];

	StringTemplateParam params[] = {
		{ .key = (String) "platform",
		  .value = { .stringValue = platform_str } },
	};
	voidResult tmpl = fun_string_template((String) "build-{platform}.bat",
										  params, 1, script_path_buffer,
										  sizeof(script_path_buffer));
	if (fun_error_is_error(tmpl.error)) {
		result.status = BUILD_DETECTED_ERROR;
		result.script_path = (String) "";
		result.error_message = (String) "Platform string too long";
		return result;
	}

	result.script_path = (String)script_path_buffer;

	if (build_script_exists(result.script_path)) {
		result.status = BUILD_DETECTED_EXISTS;
		result.error_message = (String) "";
	} else {
		result.status = BUILD_DETECTED_MISSING;
		result.error_message = (String) "";
	}

	return result;
}
