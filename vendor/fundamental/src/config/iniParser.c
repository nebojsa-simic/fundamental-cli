/*
 * INI file parser for the config module.
 *
 * Parses flat key=value INI files in-place (modifies buffer by inserting
 * null terminators). Keys and values stored in the hashmap point directly
 * into the modified buffer.
 *
 * Supported format:
 *   key = value          (whitespace around '=' is trimmed)
 *   key=value            (no spaces)
 *   key = "quoted value" (quotes are stripped)
 *   ; comment            (lines starting with ; are ignored)
 *   # comment            (lines starting with # are ignored)
 *   (blank lines ignored)
 */

#include "fundamental/config/config.h"
#include "fundamental/error/error.h"
#include "fundamental/hashmap/hashmap.h"
#include "fundamental/string/string.h"

/* Forward declaration: hash/equals for char** string keys (defined in config.c) */
uint64_t config_map_hash_string(const void *key);
bool config_map_equals_string(const void *k1, const void *k2);

/* Trim leading and trailing whitespace from a mutable string in-place */
static void ini_trim(char *s)
{
	if (!s || !*s)
		return;

	/* Trim trailing whitespace */
	size_t len = fun_string_length(s);
	while (len > 0 &&
		   (s[len - 1] == ' ' || s[len - 1] == '\t' || s[len - 1] == '\r')) {
		s[--len] = '\0';
	}

	/* Trim leading whitespace by shifting left */
	size_t start = 0;
	while (s[start] == ' ' || s[start] == '\t')
		start++;
	if (start > 0) {
		size_t i = 0;
		while (s[start + i]) {
			s[i] = s[start + i];
			i++;
		}
		s[i] = '\0';
	}
}

/* Strip surrounding double quotes from a value string */
static void ini_strip_quotes(char *s)
{
	if (!s || !*s)
		return;
	size_t len = fun_string_length(s);
	if (len >= 2 && s[0] == '"' && s[len - 1] == '"') {
		/* Shift content left by 1, remove trailing quote */
		for (size_t i = 0; i < len - 2; i++) {
			s[i] = s[i + 1];
		}
		s[len - 2] = '\0';
	}
}

/*
 * Parse INI content in-place, storing key→value pairs in out_pairs.
 *
 * Modifies 'content' by inserting null terminators.
 * out_pairs must be a valid HashMap (string keys, string values).
 * Malformed lines are silently skipped; only out-of-memory is a hard error.
 */
ErrorResult fun_ini_parse(char *content, size_t content_len, HashMap *out_pairs)
{
	if (!content || !out_pairs)
		return ERROR_RESULT_NULL_POINTER;

	/* Null-terminate the buffer */
	if (content_len > 0)
		content[content_len] = '\0';

	char *pos = content;
	char *end = content + content_len;

	while (pos < end && *pos) {
		/* Find end of line */
		char *line_start = pos;
		char *newline = pos;
		while (newline < end && *newline && *newline != '\n')
			newline++;

		/* Null-terminate the line at newline (or end of buffer) */
		char *line_end = newline;
		/* Remove trailing \r for Windows line endings */
		if (line_end > line_start && *(line_end - 1) == '\r')
			line_end--;
		*line_end = '\0';

		/* Advance past the newline for next iteration */
		pos = (*newline == '\n') ? newline + 1 : newline;

		/* Trim leading whitespace to check first char */
		char *line = line_start;
		while (*line == ' ' || *line == '\t')
			line++;

		/* Skip blank lines and comment lines */
		if (*line == '\0' || *line == ';' || *line == '#')
			continue;

		/* Find '=' separator */
		char *eq = line;
		while (*eq && *eq != '=')
			eq++;

		if (*eq != '=') {
			/* No '=' found - malformed line, skip gracefully */
			continue;
		}

		/* Split at '=': null-terminate the key */
		*eq = '\0';
		char *key = line;
		char *value = eq + 1;

		/* Trim key and value */
		ini_trim(key);
		ini_trim(value);

		/* Strip quotes from value */
		ini_strip_quotes(value);

		/* Skip if key is empty after trimming */
		if (!*key)
			continue;

		/* Store key→value in the map (pointers into modified buffer) */
		ErrorResult put_err = fun_hashmap_put(out_pairs, &key, &value);
		if (fun_error_is_error(put_err)) {
			/* Only critical failure: out of memory */
			return put_err;
		}
	}

	return ERROR_RESULT_NO_ERROR;
}
