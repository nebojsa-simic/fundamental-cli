#ifndef BUILD_CONFIG_H
#define BUILD_CONFIG_H

#include "vendor/fundamental/include/string/string.h"
#include "vendor/fundamental/include/error/error.h"

/**
 * Build configuration module
 * Parses [build] section from fun.ini
 */

#define MAX_CONFIG_VALUE 256

/**
 * Build configuration structure
 */
typedef struct {
	String entry_point;
	String flags;
	String standard;
	String output;
	int use_nostdlib;
} BuildConfig;

/**
 * Parse build configuration from fun.ini
 * @return BuildConfig with parsed values
 */
BuildConfig build_config_load(void);

/**
 * Get entry point from config
 * @param config BuildConfig
 * @return String with entry point path
 */
String build_config_get_entry(BuildConfig config);

/**
 * Get custom flags from config
 * @param config BuildConfig
 * @return String with compiler flags
 */
String build_config_get_flags(BuildConfig config);

#endif // BUILD_CONFIG_H
