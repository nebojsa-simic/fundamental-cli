/*
 * Config module core implementation.
 *
 * Manages configuration from three cascading sources:
 *   CLI args  (--config:key=value)  - highest priority
 *   Env vars  (APPNAME_KEY)         - middle priority
 *   INI file  ({exe_dir}/{app}.ini) - lowest priority
 */

#include "fundamental/config/config.h"
#include "fundamental/error/error.h"
#include "fundamental/hashmap/hashmap.h"
#include "fundamental/memory/memory.h"
#include "fundamental/string/string.h"
#include "fundamental/filesystem/filesystem.h"

/* ------------------------------------------------------------------
 * Platform functions (implemented in arch/config/{platform}/env.c)
 * ------------------------------------------------------------------ */

/* Look up a fully-transformed env var name (e.g., "MYAPP_DATABASE_HOST").
 * Returns 0 on success, -1 if not found. */
int fun_platform_env_lookup(const char *env_var_name, char *out_buf,
							size_t buf_size);

/* Get the directory of the running executable.
 * Returns 0 on success, -1 on error. */
int fun_platform_get_executable_dir(char *out_dir, size_t buf_size);

/* Read entire text file into buffer. Null-terminates result.
 * Returns 0 on success, -1 file not found, -2 other error. */
int fun_platform_read_text_file(const char *path, char *buffer, size_t max_size,
								size_t *out_bytes_read);

/* ------------------------------------------------------------------
 * Internal function declarations (implemented in iniParser.c / cliParser.c)
 * ------------------------------------------------------------------ */

ErrorResult fun_ini_parse(char *content, size_t content_len,
						  HashMap *out_pairs);

ErrorResult fun_cli_parse_args(int argc, const char **argv, HashMap *out_pairs,
							   Memory pool, size_t pool_size,
							   size_t *pool_used);

/* ------------------------------------------------------------------
 * HashMap helpers for string keys (char**)
 * The hashmap copies sizeof(char*) bytes from &key_ptr, storing the pointer.
 * These functions dereference the stored char** to get the actual string.
 * ------------------------------------------------------------------ */

uint64_t config_map_hash_string(const void *key)
{
	const char *str = *(const char **)key;
	if (!str)
		return 0;
	uint64_t hash = 14695981039346656037ULL;
	while (*str) {
		hash ^= (uint8_t)*str++;
		hash *= 1099511628211ULL;
	}
	return hash;
}

bool config_map_equals_string(const void *k1, const void *k2)
{
	const char *s1 = *(const char **)k1;
	const char *s2 = *(const char **)k2;
	if (!s1 || !s2)
		return s1 == s2;
	while (*s1 && (*s1 == *s2)) {
		s1++;
		s2++;
	}
	return *s1 == *s2;
}

/* Create a string→string HashMap using the config hash/equals functions */
static HashMapResult config_create_map(void)
{
	return fun_hashmap_create(sizeof(char *), sizeof(char *), 16,
							  config_map_hash_string, config_map_equals_string);
}

/* Look up a string key in a HashMap. Returns pointer to value or NULL. */
static const char *config_map_get(const HashMap *map, const char *key)
{
	char *out_val = NULL;
	ErrorResult err = fun_hashmap_get(map, &key, &out_val);
	if (fun_error_is_error(err))
		return NULL;
	return out_val;
}

/* ------------------------------------------------------------------
 * String helpers (no stdlib)
 * ------------------------------------------------------------------ */

static size_t cfg_strlen(const char *s)
{
	if (!s)
		return 0;
	size_t n = 0;
	while (s[n])
		n++;
	return n;
}

/* Copy src into dst (up to dst_size-1 chars, always null-terminates) */
static void cfg_strncpy(char *dst, const char *src, size_t dst_size)
{
	if (!dst || !src || dst_size == 0)
		return;
	size_t i = 0;
	while (i < dst_size - 1 && src[i]) {
		dst[i] = src[i];
		i++;
	}
	dst[i] = '\0';
}

/* Build env var name: "{APPNAME}_{KEY}" with dots→underscores, uppercase */
static void config_build_env_name(const char *app_name, const char *key,
								  char *out_buf, size_t buf_size)
{
	if (!app_name || !key || !out_buf || buf_size == 0)
		return;

	size_t i = 0;

	/* Uppercase app_name */
	for (size_t j = 0; app_name[j] && i < buf_size - 1; j++) {
		char c = app_name[j];
		if (c >= 'a' && c <= 'z')
			c = (char)(c - 32);
		out_buf[i++] = c;
	}

	/* Append '_' separator */
	if (i < buf_size - 1)
		out_buf[i++] = '_';

	/* Uppercase key, replacing dots with underscores */
	for (size_t j = 0; key[j] && i < buf_size - 1; j++) {
		char c = key[j];
		if (c >= 'a' && c <= 'z')
			c = (char)(c - 32);
		if (c == '.')
			c = '_';
		out_buf[i++] = c;
	}

	out_buf[i] = '\0';
}

/* Write a string into a pool buffer. Returns pointer to copy or NULL. */
static char *pool_write(Memory pool, size_t pool_size, size_t *used,
						const char *src, size_t len)
{
	if (*used + len + 1 > pool_size)
		return NULL;
	char *dst = (char *)pool + *used;
	for (size_t i = 0; i < len; i++)
		dst[i] = src[i];
	dst[len] = '\0';
	*used += len + 1;
	return dst;
}

/* ------------------------------------------------------------------
 * Boolean and integer value parsers (tasks 65-74)
 * ------------------------------------------------------------------ */

/* Case-insensitive compare of s against target (target must be lowercase) */
static bool cfg_iequal(const char *s, const char *target)
{
	while (*target) {
		char c = *s;
		if (c >= 'A' && c <= 'Z')
			c = (char)(c + 32);
		if (c != *target)
			return false;
		s++;
		target++;
	}
	return *s == '\0';
}

/* Parse boolean string value. Returns error on invalid input. */
static ErrorResult fun_parse_bool(const char *value, bool *out_result)
{
	if (!value || !out_result)
		return ERROR_RESULT_NULL_POINTER;

	if (cfg_iequal(value, "true") || cfg_iequal(value, "1") ||
		cfg_iequal(value, "yes")) {
		*out_result = true;
		return ERROR_RESULT_NO_ERROR;
	}

	if (cfg_iequal(value, "false") || cfg_iequal(value, "0") ||
		cfg_iequal(value, "no")) {
		*out_result = false;
		return ERROR_RESULT_NO_ERROR;
	}

	return ERROR_RESULT_CONFIG_PARSE_ERROR;
}

/* Parse decimal integer string value. Returns error on invalid input. */
static ErrorResult fun_parse_int(const char *value, int64_t *out_result)
{
	if (!value || !out_result)
		return ERROR_RESULT_NULL_POINTER;

	const char *p = value;
	int64_t result = 0;
	int negative = 0;

	if (*p == '-') {
		negative = 1;
		p++;
	} else if (*p == '+') {
		p++;
	}

	/* Must have at least one digit */
	if (*p < '0' || *p > '9')
		return ERROR_RESULT_CONFIG_PARSE_ERROR;

	const char *start = p;
	while (*p >= '0' && *p <= '9') {
		int64_t digit = *p - '0';
		/* Simple overflow check (not exhaustive) */
		if (result > (9223372036854775807LL - digit) / 10)
			return ERROR_RESULT_CONFIG_PARSE_ERROR;
		result = result * 10 + digit;
		p++;
	}

	/* Trailing non-digit characters make it invalid */
	if (*p != '\0')
		return ERROR_RESULT_CONFIG_PARSE_ERROR;

	(void)start; /* suppress unused warning */
	*out_result = negative ? -result : result;
	return ERROR_RESULT_NO_ERROR;
}

/* ------------------------------------------------------------------
 * Cascade lookup: CLI → env → INI
 * Returns pointer to value string, or NULL if not found.
 * For env hits, copies value into env_pool and caches in env_map.
 * ------------------------------------------------------------------ */
static const char *config_cascade_lookup(Config *config, const char *key)
{
	/* 1. Check CLI map */
	const char *val = config_map_get(&config->cli_map, key);
	if (val)
		return val;

	/* 2. Check env var */
	if (config->env_pool_valid) {
		/* Check env cache first */
		val = config_map_get(&config->env_map, key);
		if (val)
			return val;

		/* Transform key to env var name and look up */
		char env_name[512];
		config_build_env_name(config->app_name_buf, key, env_name,
							  sizeof(env_name));

		char env_buf[1024];
		if (fun_platform_env_lookup(env_name, env_buf, sizeof(env_buf)) == 0) {
			/* Cache the result: copy key and value into env_pool */
			size_t key_len = cfg_strlen(key);
			size_t val_len = cfg_strlen(env_buf);

			char *key_copy = pool_write(config->env_pool, CONFIG_ENV_POOL_SIZE,
										&config->env_pool_used, key, key_len);
			char *val_copy = pool_write(config->env_pool, CONFIG_ENV_POOL_SIZE,
										&config->env_pool_used, env_buf,
										val_len);

			if (key_copy && val_copy) {
				fun_hashmap_put(&config->env_map, &key_copy, &val_copy);
				return val_copy;
			}
		}
	}

	/* 3. Check INI map */
	if (config->ini_map_valid) {
		val = config_map_get(&config->ini_map, key);
		if (val)
			return val;
	}

	return NULL;
}

/* ------------------------------------------------------------------
 * fun_config_load (tasks 15-16, 2.2-2.4, 8.1-8.5)
 * ------------------------------------------------------------------ */
ConfigResult fun_config_load(String app_name, int argc, const char **argv)
{
	ConfigResult result;
	result.error = ERROR_RESULT_NO_ERROR;

	/* Validate app_name */
	if (!app_name) {
		result.error = ERROR_RESULT_NULL_POINTER;
		return result;
	}
	if (*app_name == '\0') {
		result.error = ERROR_RESULT_CONFIG_INVALID_APP_NAME;
		return result;
	}

	Config *cfg = &result.value;

	/* Zero-initialize flags */
	cfg->cli_map_valid = false;
	cfg->ini_map_valid = false;
	cfg->env_map_valid = false;
	cfg->cli_pool_valid = false;
	cfg->ini_buffer_valid = false;
	cfg->env_pool_valid = false;
	cfg->cli_pool_used = 0;
	cfg->env_pool_used = 0;
	cfg->cli_pool = NULL;
	cfg->ini_buffer = NULL;
	cfg->env_pool = NULL;

	/* Store app name */
	cfg_strncpy(cfg->app_name_buf, app_name, CONFIG_APP_NAME_MAX);

	/* Create CLI map */
	HashMapResult cli_hm = config_create_map();
	if (fun_error_is_error(cli_hm.error)) {
		result.error = cli_hm.error;
		return result;
	}
	cfg->cli_map = cli_hm.value;
	cfg->cli_map_valid = true;

	/* Create INI map */
	HashMapResult ini_hm = config_create_map();
	if (fun_error_is_error(ini_hm.error)) {
		fun_hashmap_destroy(&cfg->cli_map);
		result.error = ini_hm.error;
		return result;
	}
	cfg->ini_map = ini_hm.value;
	cfg->ini_map_valid = true;

	/* Create env map */
	HashMapResult env_hm = config_create_map();
	if (fun_error_is_error(env_hm.error)) {
		fun_hashmap_destroy(&cfg->cli_map);
		fun_hashmap_destroy(&cfg->ini_map);
		result.error = env_hm.error;
		return result;
	}
	cfg->env_map = env_hm.value;
	cfg->env_map_valid = true;

	/* Allocate CLI pool */
	MemoryResult cli_pool = fun_memory_allocate(CONFIG_CLI_POOL_SIZE);
	if (fun_error_is_error(cli_pool.error)) {
		fun_hashmap_destroy(&cfg->cli_map);
		fun_hashmap_destroy(&cfg->ini_map);
		fun_hashmap_destroy(&cfg->env_map);
		result.error = cli_pool.error;
		return result;
	}
	cfg->cli_pool = cli_pool.value;
	cfg->cli_pool_valid = true;

	/* Allocate env pool */
	MemoryResult env_pool = fun_memory_allocate(CONFIG_ENV_POOL_SIZE);
	if (fun_error_is_error(env_pool.error)) {
		fun_hashmap_destroy(&cfg->cli_map);
		fun_hashmap_destroy(&cfg->ini_map);
		fun_hashmap_destroy(&cfg->env_map);
		fun_memory_free(&cfg->cli_pool);
		result.error = env_pool.error;
		return result;
	}
	cfg->env_pool = env_pool.value;
	cfg->env_pool_valid = true;

	/* Parse CLI arguments */
	if (argc > 0 && argv) {
		fun_cli_parse_args(argc, argv, &cfg->cli_map, cfg->cli_pool,
						   CONFIG_CLI_POOL_SIZE, &cfg->cli_pool_used);
	}

	/* Load and parse INI file */
	char exe_dir[CONFIG_PATH_MAX];
	if (fun_platform_get_executable_dir(exe_dir, sizeof(exe_dir)) == 0) {
		/* Build INI filename: {app_name}.ini (single Path component) */
		char ini_filename[CONFIG_PATH_MAX];
		size_t app_len = cfg_strlen(app_name);
		if (app_len + 4 < CONFIG_PATH_MAX) {
			for (size_t i = 0; i < app_len; i++)
				ini_filename[i] = app_name[i];
			ini_filename[app_len] = '.';
			ini_filename[app_len + 1] = 'i';
			ini_filename[app_len + 2] = 'n';
			ini_filename[app_len + 3] = 'i';
			ini_filename[app_len + 4] = '\0';
		} else {
			ini_filename[0] = '\0';
		}

		/* Build exe_dir Path */
		char exe_dir_buf[CONFIG_PATH_MAX];
		const char *exe_dir_comps[16];
		Path exe_dir_path = { exe_dir_comps, 0, false };
		fun_path_from_cstr(exe_dir, exe_dir_buf, sizeof(exe_dir_buf),
						   &exe_dir_path);

		/* Join exe_dir + ini_filename → full ini Path */
		const char *ini_comp[] = { ini_filename };
		Path ini_file_path = { ini_comp, 1, false };
		const char *joined_comps[20];
		Path joined_path = { joined_comps, 0, false };
		fun_path_join(exe_dir_path, ini_file_path, &joined_path);

		/* Convert to string for platform read call */
		char ini_path[CONFIG_PATH_MAX];
		fun_path_to_string(joined_path, ini_path, sizeof(ini_path));

		/* Check if INI file exists */
		boolResult exists = fun_file_exists(joined_path);
		if (fun_error_is_ok(exists.error) && exists.value) {
			/* Allocate INI buffer */
			MemoryResult ini_buf =
				fun_memory_allocate(CONFIG_INI_BUFFER_SIZE + 1);
			if (fun_error_is_ok(ini_buf.error)) {
				cfg->ini_buffer = ini_buf.value;
				cfg->ini_buffer_valid = true;

				size_t bytes_read = 0;
				int read_result = fun_platform_read_text_file(
					ini_path, (char *)cfg->ini_buffer, CONFIG_INI_BUFFER_SIZE,
					&bytes_read);
				if (read_result == 0 && bytes_read > 0) {
					/* Parse INI content in-place */
					fun_ini_parse((char *)cfg->ini_buffer, bytes_read,
								  &cfg->ini_map);
				}
			}
		}
	}

	return result;
}

/* ------------------------------------------------------------------
 * fun_config_get_string (tasks 21, 2.8)
 * ------------------------------------------------------------------ */
StringResult fun_config_get_string(Config *config, String key)
{
	StringResult result;
	result.value = NULL;

	if (!config || !key) {
		result.error = ERROR_RESULT_NULL_POINTER;
		return result;
	}

	const char *val = config_cascade_lookup(config, key);
	if (!val) {
		result.error = ERROR_RESULT_CONFIG_KEY_NOT_FOUND;
		return result;
	}

	result.value = val;
	result.error = ERROR_RESULT_NO_ERROR;
	return result;
}

/* ------------------------------------------------------------------
 * fun_config_get_int (tasks 22, 2.9)
 * ------------------------------------------------------------------ */
int64_tResult fun_config_get_int(Config *config, String key)
{
	int64_tResult result;
	result.value = 0;

	if (!config || !key) {
		result.error = ERROR_RESULT_NULL_POINTER;
		return result;
	}

	const char *val = config_cascade_lookup(config, key);
	if (!val) {
		result.error = ERROR_RESULT_CONFIG_KEY_NOT_FOUND;
		return result;
	}

	int64_t parsed = 0;
	result.error = fun_parse_int(val, &parsed);
	result.value = parsed;
	return result;
}

/* ------------------------------------------------------------------
 * fun_config_get_bool (tasks 23, 2.10)
 * ------------------------------------------------------------------ */
boolResult fun_config_get_bool(Config *config, String key)
{
	boolResult result;
	result.value = false;

	if (!config || !key) {
		result.error = ERROR_RESULT_NULL_POINTER;
		return result;
	}

	const char *val = config_cascade_lookup(config, key);
	if (!val) {
		result.error = ERROR_RESULT_CONFIG_KEY_NOT_FOUND;
		return result;
	}

	bool parsed = false;
	result.error = fun_parse_bool(val, &parsed);
	result.value = parsed;
	return result;
}

/* ------------------------------------------------------------------
 * get_or_default wrappers (tasks 24, 2.11)
 * ------------------------------------------------------------------ */
StringResult fun_config_get_string_or_default(Config *config, String key,
											  String default_value)
{
	StringResult result = fun_config_get_string(config, key);
	if (result.error.code == ERROR_CODE_CONFIG_KEY_NOT_FOUND) {
		result.value = default_value;
		result.error = ERROR_RESULT_NO_ERROR;
	}
	return result;
}

int64_tResult fun_config_get_int_or_default(Config *config, String key,
											int64_t default_value)
{
	int64_tResult result = fun_config_get_int(config, key);
	if (result.error.code == ERROR_CODE_CONFIG_KEY_NOT_FOUND) {
		result.value = default_value;
		result.error = ERROR_RESULT_NO_ERROR;
	}
	return result;
}

boolResult fun_config_get_bool_or_default(Config *config, String key,
										  bool default_value)
{
	boolResult result = fun_config_get_bool(config, key);
	if (result.error.code == ERROR_CODE_CONFIG_KEY_NOT_FOUND) {
		result.value = default_value;
		result.error = ERROR_RESULT_NO_ERROR;
	}
	return result;
}

/* ------------------------------------------------------------------
 * fun_config_has (tasks 25, 2.12)
 * ------------------------------------------------------------------ */
boolResult fun_config_has(Config *config, String key)
{
	boolResult result;
	result.error = ERROR_RESULT_NO_ERROR;
	result.value = false;

	if (!config || !key) {
		result.error = ERROR_RESULT_NULL_POINTER;
		return result;
	}

	result.value = (config_cascade_lookup(config, key) != NULL);
	return result;
}

/* ------------------------------------------------------------------
 * fun_config_destroy (tasks 26, 2.13, 81-86)
 * ------------------------------------------------------------------ */
voidResult fun_config_destroy(Config *config)
{
	voidResult result;
	result.error = ERROR_RESULT_NO_ERROR;

	if (!config)
		return result;

	/* Destroy hashmaps (frees bucket allocations) */
	if (config->cli_map_valid) {
		fun_hashmap_destroy(&config->cli_map);
		config->cli_map_valid = false;
	}
	if (config->ini_map_valid) {
		fun_hashmap_destroy(&config->ini_map);
		config->ini_map_valid = false;
	}
	if (config->env_map_valid) {
		fun_hashmap_destroy(&config->env_map);
		config->env_map_valid = false;
	}

	/* Free memory pools */
	if (config->cli_pool_valid) {
		fun_memory_free(&config->cli_pool);
		config->cli_pool_valid = false;
	}
	if (config->ini_buffer_valid) {
		fun_memory_free(&config->ini_buffer);
		config->ini_buffer_valid = false;
	}
	if (config->env_pool_valid) {
		fun_memory_free(&config->env_pool);
		config->env_pool_valid = false;
	}

	return result;
}
