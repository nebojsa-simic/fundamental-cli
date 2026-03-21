#include "build/build.h"
#include "fundamental/file/file.h"
#include "fundamental/filesystem/filesystem.h"
#include "fundamental/async/async.h"
#include "fundamental/memory/memory.h"
#include "fundamental/console/console.h"
#include "fundamental/string/string.h"

/**
 * Find a value for a key in INI content
 */
static String find_ini_value(const char *content, const char *section,
							 const char *key)
{
	static char value_buffer[MAX_CONFIG_VALUE];

	// Find section
	const char *section_start = 0;
	const char *ptr = content;

	while (*ptr) {
		// Look for [section]
		if (*ptr == '[') {
			const char *section_ptr = ptr + 1;
			const char *section_end = section_ptr;
			while (*section_end && *section_end != ']') {
				section_end++;
			}

			// Check if this is our section
			StringLength section_len = section_end - section_ptr;
			StringLength search_len = fun_string_length((String)section);

			if (section_len == search_len) {
				// Compare section names
				int match = 1;
				for (StringLength i = 0; i < section_len; i++) {
					if (section_ptr[i] != section[i]) {
						match = 0;
						break;
					}
				}

				if (match) {
					section_start = ptr;
					break;
				}
			}

			ptr = section_end;
		}
		ptr++;
	}

	if (!section_start) {
		return (String) "";
	}

	// Find key in section
	ptr = section_start;
	while (*ptr && *ptr != '[') {
		// Look for key = value
		const char *key_start = ptr;
		while (*ptr && *ptr != '=' && *ptr != '\n') {
			ptr++;
		}

		if (*ptr == '=') {
			// Found potential key
			StringLength key_len = ptr - key_start;
			StringLength search_len = fun_string_length((String)key);

			// Trim whitespace from key
			while (key_len > 0 && (key_start[key_len - 1] == ' ' ||
								   key_start[key_len - 1] == '\t')) {
				key_len--;
			}

			if (key_len == search_len) {
				// Compare key names
				int match = 1;
				for (StringLength i = 0; i < key_len; i++) {
					if (key_start[i] != key[i]) {
						match = 0;
						break;
					}
				}

				if (match) {
					// Found the key, get value
					ptr++; // skip '='

					// Skip leading whitespace
					while (*ptr == ' ' || *ptr == '\t') {
						ptr++;
					}

					// Copy value
					StringLength value_len = 0;
					const char *value_start = ptr;
					while (*ptr && *ptr != '\n' && *ptr != '\r' &&
						   value_len < MAX_CONFIG_VALUE - 1) {
						value_buffer[value_len++] = *ptr++;
					}
					value_buffer[value_len] = '\0';

					// Trim trailing whitespace
					while (value_len > 0 &&
						   (value_buffer[value_len - 1] == ' ' ||
							value_buffer[value_len - 1] == '\t')) {
						value_buffer[--value_len] = '\0';
					}

					return (String)value_buffer;
				}
			}
		}

		// Move to next line
		while (*ptr && *ptr != '\n') {
			ptr++;
		}
		if (*ptr == '\n') {
			ptr++;
		}
	}

	return (String) "";
}

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

	// Try to read fun.ini
	char _ini_buf[64];
	const char *_ini_comps[4];
	Path _ini_path = { _ini_comps, 0, false };
	fun_path_from_cstr("fun.ini", _ini_buf, sizeof(_ini_buf), &_ini_path);
	boolResult ini_exists = fun_file_exists(_ini_path);
	if (fun_error_is_error(ini_exists.error) || !ini_exists.value) {
		return config;
	}

	static char ini_buf[512];
	fun_memory_fill((Memory)ini_buf, sizeof(ini_buf), 0);

	uint64_t ini_file_size;
	voidResult sz = fun_file_size(_ini_path, &ini_file_size);
	if (fun_error_is_error(sz.error)) {
		return config;
	}
	size_t bytes_to_read = (ini_file_size < sizeof(ini_buf) - 1)
							   ? (size_t)ini_file_size
							   : sizeof(ini_buf) - 1;
	AsyncResult r =
		fun_read_file_in_memory((Read){ .file_path = (String) "fun.ini",
										.output = (Memory)ini_buf,
										.bytes_to_read = bytes_to_read });
	fun_async_await(&r, -1);
	if (r.status != ASYNC_COMPLETED) {
		return config;
	}

	const char *content = (const char *)ini_buf;

	// Parse [build] section
	config.entry_point = find_ini_value(content, "build", "entry");
	config.flags = find_ini_value(content, "build", "flags");
	config.standard = find_ini_value(content, "build", "standard");
	config.output = find_ini_value(content, "build", "output");

	// Check for nostdlib in standard
	if (fun_string_length(config.standard) > 0) {
		if (fun_string_index_of(config.standard, (String) "-nostdlib", 0) >=
			0) {
			config.use_nostdlib = 1;
		}
	}

	// Also check global [build] section
	if (fun_string_length(config.entry_point) == 0) {
		// Try global entry
		config.entry_point = find_ini_value(content, "", "entry");
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
