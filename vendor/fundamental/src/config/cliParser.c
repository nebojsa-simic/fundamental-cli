/*
 * CLI argument parser for the config module.
 *
 * Scans argv for arguments matching --config:key=value format.
 * Keys and values are copied into a caller-provided pool buffer
 * since argv strings cannot be null-terminated at arbitrary positions.
 *
 * Format:
 *   --config:key=value           (simple value)
 *   --config:key="value here"    (quoted value, quotes stripped)
 *   --config:key=                (empty value allowed)
 *
 * Rules:
 *   - Only arguments starting with "--config:" are processed.
 *   - Non-config arguments are silently skipped.
 *   - Malformed --config arguments (no '=') are silently skipped.
 *   - Later arguments override earlier arguments for the same key.
 */

#include "../../include/config/config.h"
#include "../../include/error/error.h"
#include "../../include/hashmap/hashmap.h"
#include "../../include/string/string.h"

#define CLI_PREFIX "--config:"
#define CLI_PREFIX_LEN 9

/* Copy at most max_len chars from src into pool, null-terminate, return pointer */
static char *cli_pool_copy(Memory pool, size_t pool_size, size_t *pool_used,
						   const char *src, size_t src_len)
{
	if (*pool_used + src_len + 1 > pool_size)
		return NULL;
	char *dst = (char *)pool + *pool_used;
	for (size_t i = 0; i < src_len; i++)
		dst[i] = src[i];
	dst[src_len] = '\0';
	*pool_used += src_len + 1;
	return dst;
}

/* Strip surrounding double quotes from value (in pool, modifiable) */
static void cli_strip_quotes(char *s)
{
	if (!s || !*s)
		return;
	size_t len = fun_string_length(s);
	if (len >= 2 && s[0] == '"' && s[len - 1] == '"') {
		for (size_t i = 0; i < len - 2; i++)
			s[i] = s[i + 1];
		s[len - 2] = '\0';
	}
}

/*
 * Parse CLI arguments for --config:key=value patterns.
 *
 * @param argc        Argument count.
 * @param argv        Argument vector.
 * @param out_pairs   Initialized HashMap to receive key→value pairs.
 * @param pool        Memory pool for key/value string copies.
 * @param pool_size   Size of pool in bytes.
 * @param pool_used   In/out: bytes currently used in pool.
 */
ErrorResult fun_cli_parse_args(int argc, const char **argv, HashMap *out_pairs,
							   Memory pool, size_t pool_size, size_t *pool_used)
{
	if (!out_pairs || !pool || !pool_used)
		return ERROR_RESULT_NULL_POINTER;
	if (argc <= 0 || !argv)
		return ERROR_RESULT_NO_ERROR;

	for (int i = 0; i < argc; i++) {
		if (!argv[i])
			continue;

		/* Check if argument starts with "--config:" */
		const char *arg = argv[i];
		bool matches = true;
		for (size_t j = 0; j < CLI_PREFIX_LEN; j++) {
			if (arg[j] != CLI_PREFIX[j]) {
				matches = false;
				break;
			}
		}
		if (!matches)
			continue;

		/* arg points to "key=value" after the prefix */
		const char *key_start = arg + CLI_PREFIX_LEN;

		/* Find '=' separator */
		const char *eq = key_start;
		while (*eq && *eq != '=')
			eq++;

		if (*eq != '=') {
			/* Malformed: no '=' - skip gracefully */
			continue;
		}

		size_t key_len = (size_t)(eq - key_start);
		const char *val_start = eq + 1;
		size_t val_len = fun_string_length(val_start);

		/* Skip args with empty key */
		if (key_len == 0)
			continue;

		/* Copy key and value into pool */
		char *key_copy =
			cli_pool_copy(pool, pool_size, pool_used, key_start, key_len);
		if (!key_copy)
			continue; /* Pool full - skip this arg */

		char *val_copy =
			cli_pool_copy(pool, pool_size, pool_used, val_start, val_len);
		if (!val_copy)
			continue; /* Pool full - skip this arg */

		/* Strip quotes from value */
		cli_strip_quotes(val_copy);

		/* Store in map (later args override earlier for same key) */
		fun_hashmap_put(out_pairs, &key_copy, &val_copy);
	}

	return ERROR_RESULT_NO_ERROR;
}
