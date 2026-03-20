#ifndef LIBRARY_CONFIG_H
#define LIBRARY_CONFIG_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "../error/error.h"
#include "../hashmap/hashmap.h"
#include "../memory/memory.h"
#include "../string/string.h"

/*
 * Config Module
 *
 * Provides cascading configuration management for CLI applications built
 * without stdlib dependencies. Sources are checked in priority order:
 *
 *   1. Command-line: --config:key=value  (highest priority)
 *   2. Environment:  APPNAME_KEY (dots become underscores, uppercase)
 *   3. INI file:     {exe_dir}/{app_name}.ini (lowest priority)
 *
 * CLI format:  --config:database.host=localhost
 *              --config:server.port=5432
 * Env format:  MYAPP_DATABASE_HOST=localhost  (for app_name "myapp")
 *              MYAPP_SERVER_PORT=5432
 * INI format:  database.host = localhost
 *              ; lines starting with ; or # are comments
 *              server.port = 5432
 *
 * Usage example:
 *   ConfigResult cfg = fun_config_load("myapp", argc, argv);
 *   if (fun_error_is_error(cfg.error)) { return 1; }
 *
 *   StringResult host = fun_config_get_string(&cfg.value, "database.host");
 *   if (fun_error_is_ok(host.error)) { use(host.value); }
 *
 *   int64_t port = fun_config_get_int_or_default(&cfg.value, "server.port", 8080).value;
 *
 *   fun_config_destroy(&cfg.value);
 *
 * Boolean values: "true"/"false", "1"/"0", "yes"/"no" (case-insensitive)
 * Integer values: decimal integers, negative integers supported
 * All keys are case-sensitive.
 */

/* Buffer size limits */
#define CONFIG_APP_NAME_MAX 256
#define CONFIG_INI_BUFFER_SIZE 65536
#define CONFIG_CLI_POOL_SIZE 8192
#define CONFIG_ENV_POOL_SIZE 8192
#define CONFIG_PATH_MAX 512

/*
 * Config struct - treat as opaque, always use API functions.
 * Do not copy this struct; pass pointers to all API functions.
 * Call fun_config_destroy() when done to release all memory.
 */
typedef struct {
	char app_name_buf[CONFIG_APP_NAME_MAX]; /* Owned copy of app name */
	HashMap cli_map; /* CLI args key→value */
	HashMap ini_map; /* INI file key→value */
	HashMap env_map; /* Env var cache key→value */
	Memory cli_pool; /* Pool for CLI string copies */
	size_t cli_pool_used; /* Bytes used in cli_pool */
	Memory ini_buffer; /* INI file content (owned) */
	Memory env_pool; /* Pool for env string copies */
	size_t env_pool_used; /* Bytes used in env_pool */
	bool cli_map_valid;
	bool ini_map_valid;
	bool env_map_valid;
	bool cli_pool_valid;
	bool ini_buffer_valid;
	bool env_pool_valid;
} Config;

/* Result type for Config */
DEFINE_RESULT_TYPE(Config);

/* Result type for String values */
#ifndef STRING_RESULT_DEFINED
#define STRING_RESULT_DEFINED
DEFINE_RESULT_TYPE(String);
#endif

/*
 * Load configuration from all sources.
 *
 * Loads CLI args, environment variables, and INI file (if it exists).
 * The INI file is located at {exe_dir}/{app_name}.ini.
 * A missing INI file is not an error.
 *
 * @param app_name  Application name for env prefix (e.g., "myapp" → MYAPP_).
 *                  Must not be NULL or empty.
 * @param argc      Argument count (pass 0 to skip CLI parsing).
 * @param argv      Argument vector (pass NULL to skip CLI parsing).
 *
 * @return ConfigResult.value on success, error on NULL/empty app_name.
 *         Error codes: ERROR_CODE_NULL_POINTER,
 *                      ERROR_CODE_CONFIG_INVALID_APP_NAME
 */
CanReturnError(Config)
	fun_config_load(String app_name, int argc, const char **argv);

/*
 * Get string value by key using cascade: CLI → env → INI.
 *
 * @param config  Pointer to Config (must not be NULL).
 * @param key     Configuration key in dotted notation (e.g., "database.host").
 *
 * @return StringResult.value contains the string on success.
 *         Error codes: ERROR_CODE_CONFIG_KEY_NOT_FOUND,
 *                      ERROR_CODE_NULL_POINTER
 */
CanReturnError(String) fun_config_get_string(Config *config, String key);

/*
 * Get integer value by key using cascade: CLI → env → INI.
 *
 * @param config  Pointer to Config (must not be NULL).
 * @param key     Configuration key.
 *
 * @return int64_tResult.value contains the integer on success.
 *         Error codes: ERROR_CODE_CONFIG_KEY_NOT_FOUND,
 *                      ERROR_CODE_CONFIG_PARSE_ERROR,
 *                      ERROR_CODE_NULL_POINTER
 */
CanReturnError(int64_t) fun_config_get_int(Config *config, String key);

/*
 * Get boolean value by key using cascade: CLI → env → INI.
 *
 * Accepted true values:  "true", "1", "yes" (case-insensitive)
 * Accepted false values: "false", "0", "no" (case-insensitive)
 *
 * @param config  Pointer to Config (must not be NULL).
 * @param key     Configuration key.
 *
 * @return boolResult.value contains the boolean on success.
 *         Error codes: ERROR_CODE_CONFIG_KEY_NOT_FOUND,
 *                      ERROR_CODE_CONFIG_PARSE_ERROR,
 *                      ERROR_CODE_NULL_POINTER
 */
CanReturnError(bool) fun_config_get_bool(Config *config, String key);

/*
 * Get string value or default if key not found.
 * Never returns a KEY_NOT_FOUND error.
 *
 * @param config         Pointer to Config.
 * @param key            Configuration key.
 * @param default_value  Returned when key is absent.
 */
StringResult fun_config_get_string_or_default(Config *config, String key,
											  String default_value);

/*
 * Get integer value or default if key not found.
 * Never returns a KEY_NOT_FOUND error.
 *
 * @param config         Pointer to Config.
 * @param key            Configuration key.
 * @param default_value  Returned when key is absent.
 */
int64_tResult fun_config_get_int_or_default(Config *config, String key,
											int64_t default_value);

/*
 * Get boolean value or default if key not found.
 * Never returns a KEY_NOT_FOUND error.
 *
 * @param config         Pointer to Config.
 * @param key            Configuration key.
 * @param default_value  Returned when key is absent.
 */
boolResult fun_config_get_bool_or_default(Config *config, String key,
										  bool default_value);

/*
 * Check if a key exists in any source without retrieving its value.
 *
 * @param config  Pointer to Config.
 * @param key     Configuration key to check.
 *
 * @return boolResult.value = true if key found, false if not.
 *         No KEY_NOT_FOUND error is returned.
 */
boolResult fun_config_has(Config *config, String key);

/*
 * Free all resources held by the Config.
 *
 * After calling this, the Config is invalid. Passing NULL is a no-op.
 *
 * @param config  Pointer to Config to destroy.
 */
voidResult fun_config_destroy(Config *config);

#endif /* LIBRARY_CONFIG_H */
