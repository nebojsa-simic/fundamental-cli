#include "build/build.h"
#include "fundamental/config/config.h"
#include "fundamental/string/string.h"

static char entry_buf[MAX_CONFIG_VALUE];
static char flags_buf[MAX_CONFIG_VALUE];
static char standard_buf[MAX_CONFIG_VALUE];
static char output_buf[MAX_CONFIG_VALUE];

/**
 * Parse build configuration from fun.ini
 */
BuildConfig build_config_load(void)
{
	BuildConfig config;
	config.entry_point = (String) "";
	config.flags = (String) "";
	config.standard = (String) "";
	config.output = (String) "";
	config.use_nostdlib = 0;

	ConfigResult cfg = fun_config_load((String) "fun", 0, NULL);
	if (fun_error_is_error(cfg.error)) {
		return config;
	}

	String entry = fun_config_get_string_or_default(
					   &cfg.value, (String) "entry", (String) "")
					   .value;
	String flags = fun_config_get_string_or_default(
					   &cfg.value, (String) "flags", (String) "")
					   .value;
	String standard = fun_config_get_string_or_default(
						  &cfg.value, (String) "standard", (String) "")
						  .value;
	String output = fun_config_get_string_or_default(
						&cfg.value, (String) "output", (String) "")
						.value;

	fun_string_copy(entry, entry_buf, sizeof(entry_buf));
	fun_string_copy(flags, flags_buf, sizeof(flags_buf));
	fun_string_copy(standard, standard_buf, sizeof(standard_buf));
	fun_string_copy(output, output_buf, sizeof(output_buf));

	fun_config_destroy(&cfg.value);

	config.entry_point = (String)entry_buf;
	config.flags = (String)flags_buf;
	config.standard = (String)standard_buf;
	config.output = (String)output_buf;

	// Check for nostdlib in standard
	if (fun_string_length(config.standard) > 0) {
		if (fun_string_index_of(config.standard, (String) "-nostdlib", 0) >=
			0) {
			config.use_nostdlib = 1;
		}
	}

	return config;
}

/**
 * Get entry point from config
 */
String build_config_get_entry(BuildConfig config)
{
	if (fun_string_length(config.entry_point) > 0) {
		return config.entry_point;
	}
	// Default entry point
	return (String) "src/main.c";
}

/**
 * Get custom flags from config
 */
String build_config_get_flags(BuildConfig config)
{
	return config.flags;
}
