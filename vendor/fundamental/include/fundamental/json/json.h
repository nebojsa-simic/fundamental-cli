#ifndef LIBRARY_JSON_H
#define LIBRARY_JSON_H

#include "../error/error.h"
#include "../string/string.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define FUN_JSON_MAX_DEPTH 32

typedef enum {
	FUN_JSON_OBJECT_START,
	FUN_JSON_OBJECT_END,
	FUN_JSON_ARRAY_START,
	FUN_JSON_ARRAY_END,
	FUN_JSON_STRING,
	FUN_JSON_NUMBER,
	FUN_JSON_BOOL,
	FUN_JSON_NULL,
	FUN_JSON_KEY,
	FUN_JSON_TOKEN_END,
	FUN_JSON_INCOMPLETE
} FunJsonTokenType;

typedef struct {
	FunJsonTokenType type;
	String value;
	uint64_t length;
	uint64_t depth;
	bool parent_is_array;
	uint64_t array_index;
} FunJsonToken;

typedef struct {
	char *_data;
	uint64_t _pos;
	uint64_t _len;
	uint64_t _depth;
	uint64_t _array_index[FUN_JSON_MAX_DEPTH + 1];
	bool _in_array[FUN_JSON_MAX_DEPTH + 1];
	bool _expecting_key[FUN_JSON_MAX_DEPTH + 1];
	bool _expecting_value[FUN_JSON_MAX_DEPTH + 1];
	bool _expecting_comma[FUN_JSON_MAX_DEPTH + 1];
	uint64_t _save_pos;
	bool _streaming;
} FunJsonState;

// === Initialization (Layer 1 — mutable buffer) ===

ErrorResult fun_json_init(FunJsonState *state, char *data, uint64_t len);
ErrorResult fun_json_init_at_path(FunJsonState *state, char *data, uint64_t len,
								  String path, uint64_t *base_depth);

// === Streaming support ===

ErrorResult fun_json_feed(FunJsonState *state, uint64_t new_len);

// === Token iteration (Layer 1) ===

ErrorResult fun_json_next(FunJsonState *state, FunJsonToken *token);
ErrorResult fun_json_next_at(FunJsonState *state, uint64_t depth,
							 FunJsonToken *token);

// === Iteration helpers (Layer 1) ===

ErrorResult fun_json_skip_value(FunJsonState *state);
ErrorResult fun_json_find_key(FunJsonState *state, uint64_t depth, String key,
							  FunJsonToken *token);

// === Query (Layer 2 — non-mutating, idempotent) ===

ErrorResult fun_json_query(String data, uint64_t len, String path,
						   FunJsonToken *token);

// === Extractors (work on any token) ===

ErrorResult fun_json_token_copy_string(FunJsonToken *token, OutputString out,
									   uint64_t out_size);
int64_tResult fun_json_token_as_int(FunJsonToken *token);
doubleResult fun_json_token_as_double(FunJsonToken *token);
boolResult fun_json_token_as_bool(FunJsonToken *token);
boolResult fun_json_token_is_null(FunJsonToken *token);
bool fun_json_token_value_equals(FunJsonToken *token, String expected);

// === Convenience combinators (Layer 2 — non-mutating) ===

ErrorResult fun_json_query_string(String data, uint64_t len, String path,
								  OutputString out, uint64_t out_size);
int64_tResult fun_json_query_int(String data, uint64_t len, String path);
doubleResult fun_json_query_double(String data, uint64_t len, String path);
boolResult fun_json_query_bool(String data, uint64_t len, String path);

// === Array extractors (Layer 2 — non-mutating) ===

uint64_tResult fun_json_query_int_array(String data, uint64_t len, String path,
										int64_t *out, uint64_t max_count);
uint64_tResult fun_json_query_double_array(String data, uint64_t len,
										   String path, double *out,
										   uint64_t max_count);
uint64_tResult fun_json_query_string_array(String data, uint64_t len,
										   String path, OutputString buffer,
										   uint64_t buffer_size);

// === Iteration callback (Layer 2 — non-mutating) ===

typedef ErrorResult (*FunJsonEachFn)(FunJsonToken *element, uint64_t index,
									 void *context, FunJsonState *state);

ErrorResult fun_json_for_each(String data, uint64_t len, String path,
							  FunJsonEachFn fn, void *context);

#endif // LIBRARY_JSON_H
