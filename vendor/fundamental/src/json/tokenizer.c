#include "fundamental/json/json.h"
#include "fundamental/string/string.h"

static inline bool json_bytes_equal(const char *a, uint64_t a_len,
									const char *b, uint64_t b_len)
{
	if (a_len != b_len)
		return false;
	for (uint64_t i = 0; i < a_len; i++)
		if (a[i] != b[i])
			return false;
	return true;
}

#define JSON_HEX_DIGIT(c)                            \
	(((c) >= '0' && (c) <= '9') ? ((c) - '0') :      \
	 ((c) >= 'a' && (c) <= 'f') ? ((c) - 'a' + 10) : \
	 ((c) >= 'A' && (c) <= 'F') ? ((c) - 'A' + 10) : \
								  -1)

static inline bool json_is_digit(char c)
{
	return c >= '0' && c <= '9';
}

static inline bool json_is_whitespace(char c)
{
	return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

static void json_skip_whitespace(FunJsonState *state)
{
	while (state->_pos < state->_len &&
		   json_is_whitespace(state->_data[state->_pos]))
		state->_pos++;
}

static void json_set_token_ctx(FunJsonState *state, FunJsonToken *token)
{
	token->depth = state->_depth;
	token->parent_is_array = state->_in_array[state->_depth];
	token->array_index = state->_array_index[state->_depth];
}

#define dc(state) (state->_depth)

// === Internal parsing functions ===

static ErrorResult json_parse_string(FunJsonState *state, FunJsonToken *token,
									 bool is_key)
{
	token->type = is_key ? FUN_JSON_KEY : FUN_JSON_STRING;
	uint64_t start = state->_pos;
	state->_pos++;
	uint64_t len = 0;

	while (state->_pos < state->_len) {
		char c = state->_data[state->_pos];
		if (c == '"') {
			state->_pos++;
			token->value = &state->_data[start + 1];
			token->length = len;
			return ERROR_RESULT_NO_ERROR;
		}
		if (c == '\\') {
			state->_pos++;
			if (state->_pos >= state->_len) {
				if (state->_streaming) {
					state->_save_pos = start;
					token->type = FUN_JSON_INCOMPLETE;
					return ERROR_RESULT_NO_ERROR;
				}
				return ERROR_RESULT_JSON_UNTERMINATED_STRING;
			}
			c = state->_data[state->_pos];
			if (c == 'u')
				state->_pos += 4;
		}
		len++;
		state->_pos++;
	}
	if (state->_streaming) {
		state->_save_pos = start;
		token->type = FUN_JSON_INCOMPLETE;
		return ERROR_RESULT_NO_ERROR;
	}
	return ERROR_RESULT_JSON_UNTERMINATED_STRING;
}

static ErrorResult json_parse_number(FunJsonState *state, FunJsonToken *token)
{
#define NUM_INCOMPLETE                     \
	do {                                   \
		state->_save_pos = start;          \
		token->type = FUN_JSON_INCOMPLETE; \
		return ERROR_RESULT_NO_ERROR;      \
	} while (0)

	token->type = FUN_JSON_NUMBER;
	uint64_t start = state->_pos;
	uint64_t p = start;

	if (p < state->_len && state->_data[p] == '-')
		p++;
	if (p >= state->_len) {
		if (state->_streaming)
			NUM_INCOMPLETE;
		return ERROR_RESULT_JSON_INVALID_NUMBER;
	}

	if (state->_data[p] == '0') {
		p++;
		if (p < state->_len && json_is_digit(state->_data[p]))
			return ERROR_RESULT_JSON_INVALID_NUMBER;
	} else if (json_is_digit(state->_data[p])) {
		while (p < state->_len && json_is_digit(state->_data[p]))
			p++;
	} else {
		return ERROR_RESULT_JSON_INVALID_NUMBER;
	}

	if (p >= state->_len) {
		if (state->_streaming)
			NUM_INCOMPLETE;
		goto done;
	}

	if (state->_data[p] == '.') {
		p++;
		if (p >= state->_len) {
			if (state->_streaming)
				NUM_INCOMPLETE;
			return ERROR_RESULT_JSON_INVALID_NUMBER;
		}
		if (!json_is_digit(state->_data[p]))
			return ERROR_RESULT_JSON_INVALID_NUMBER;
		while (p < state->_len && json_is_digit(state->_data[p]))
			p++;
	}

	if (p >= state->_len) {
		if (state->_streaming)
			NUM_INCOMPLETE;
		goto done;
	}

	if (state->_data[p] == 'e' || state->_data[p] == 'E') {
		p++;
		if (p >= state->_len) {
			if (state->_streaming)
				NUM_INCOMPLETE;
			return ERROR_RESULT_JSON_INVALID_NUMBER;
		}
		if (state->_data[p] == '+' || state->_data[p] == '-')
			p++;
		if (p >= state->_len) {
			if (state->_streaming)
				NUM_INCOMPLETE;
			return ERROR_RESULT_JSON_INVALID_NUMBER;
		}
		if (!json_is_digit(state->_data[p]))
			return ERROR_RESULT_JSON_INVALID_NUMBER;
		while (p < state->_len && json_is_digit(state->_data[p]))
			p++;
	}

done:
	token->value = &state->_data[start];
	token->length = p - start;
	state->_pos = p;

	return ERROR_RESULT_NO_ERROR;
#undef NUM_INCOMPLETE
}

static ErrorResult json_parse_literal(FunJsonState *state, const char *expected,
									  uint64_t elen, FunJsonTokenType type,
									  FunJsonToken *token)
{
	for (uint64_t i = 0; i < elen; i++) {
		if (state->_pos + i >= state->_len) {
			if (state->_streaming) {
				state->_save_pos = state->_pos;
				token->type = FUN_JSON_INCOMPLETE;
				return ERROR_RESULT_NO_ERROR;
			}
			return ERROR_RESULT_JSON_UNEXPECTED_TOKEN;
		}
		if (state->_data[state->_pos + i] != expected[i])
			return ERROR_RESULT_JSON_UNEXPECTED_TOKEN;
	}
	state->_pos += elen;

	token->type = type;
	token->value = NULL;
	token->length = 0;
	return ERROR_RESULT_NO_ERROR;
}

static ErrorResult json_value(FunJsonState *state, FunJsonToken *token);

static ErrorResult json_object(FunJsonState *state, FunJsonToken *token)
{
	token->value = &state->_data[state->_pos];
	state->_pos++;
	token->type = FUN_JSON_OBJECT_START;
	state->_depth++;
	if (state->_depth > FUN_JSON_MAX_DEPTH)
		return ERROR_RESULT_JSON_NESTING_TOO_DEEP;
	state->_in_array[dc(state)] = false;
	state->_expecting_key[dc(state)] = true;
	state->_expecting_comma[dc(state)] = false;
	return ERROR_RESULT_NO_ERROR;
}

static ErrorResult json_array(FunJsonState *state, FunJsonToken *token)
{
	token->value = &state->_data[state->_pos];
	state->_pos++;
	token->type = FUN_JSON_ARRAY_START;
	state->_depth++;
	if (state->_depth > FUN_JSON_MAX_DEPTH)
		return ERROR_RESULT_JSON_NESTING_TOO_DEEP;
	state->_in_array[dc(state)] = true;
	state->_array_index[dc(state)] = 0;
	state->_expecting_value[dc(state)] = true;
	state->_expecting_comma[dc(state)] = false;
	return ERROR_RESULT_NO_ERROR;
}

static ErrorResult json_value(FunJsonState *state, FunJsonToken *token)
{
	json_skip_whitespace(state);
	if (state->_pos >= state->_len)
		return ERROR_RESULT_NO_ERROR;

	char c = state->_data[state->_pos];

	switch (c) {
	case '{':
		return json_object(state, token);
	case '[':
		return json_array(state, token);
	case '"':
		return json_parse_string(state, token, false);
	case '-':
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		return json_parse_number(state, token);
	case 't':
		return json_parse_literal(state, "true", 4, FUN_JSON_BOOL, token);
	case 'f':
		return json_parse_literal(state, "false", 5, FUN_JSON_BOOL, token);
	case 'n':
		return json_parse_literal(state, "null", 4, FUN_JSON_NULL, token);
	default:
		return ERROR_RESULT_JSON_UNEXPECTED_TOKEN;
	}
}

// === Public API ===

ErrorResult fun_json_init(FunJsonState *state, char *data, uint64_t len)
{
	if (state == NULL || data == NULL)
		return ERROR_RESULT_NULL_POINTER;

	state->_data = data;
	state->_pos = 0;
	state->_len = len;
	state->_depth = 0;
	state->_in_array[0] = false;
	state->_array_index[0] = 0;
	state->_expecting_key[0] = false;
	state->_expecting_value[0] = false;
	state->_expecting_comma[0] = false;
	state->_save_pos = 0;
	state->_streaming = false;
	return ERROR_RESULT_NO_ERROR;
}

ErrorResult fun_json_feed(FunJsonState *state, uint64_t new_len)
{
	if (state == NULL)
		return ERROR_RESULT_NULL_POINTER;

	state->_len = new_len;
	state->_streaming = true;
	if (state->_save_pos > 0) {
		state->_pos = state->_save_pos;
		state->_save_pos = 0;
	}
	return ERROR_RESULT_NO_ERROR;
}

ErrorResult fun_json_next(FunJsonState *state, FunJsonToken *token)
{
	if (state == NULL || token == NULL)
		return ERROR_RESULT_NULL_POINTER;

	json_skip_whitespace(state);
	token->value = NULL;
	token->length = 0;
	token->depth = 0;
	token->parent_is_array = false;
	token->array_index = 0;

	if (state->_pos >= state->_len) {
		if (state->_streaming && state->_save_pos > 0) {
			token->type = FUN_JSON_INCOMPLETE;
			return ERROR_RESULT_NO_ERROR;
		}
		token->type = FUN_JSON_TOKEN_END;
		return ERROR_RESULT_NO_ERROR;
	}

	char c = state->_data[state->_pos];
	ErrorResult err;

	if (state->_depth == 0) {
		err = json_value(state, token);
		if (fun_error_is_error(err))
			return err;
		json_set_token_ctx(state, token);
		return ERROR_RESULT_NO_ERROR;
	}

	if (c == '}') {
		state->_pos++;
		token->type = FUN_JSON_OBJECT_END;
		json_set_token_ctx(state, token);
		if (state->_depth > 0)
			state->_depth--;
		if (state->_depth > 0)
			state->_expecting_comma[dc(state)] = true;
		return ERROR_RESULT_NO_ERROR;
	}

	if (c == ']') {
		state->_pos++;
		token->type = FUN_JSON_ARRAY_END;
		json_set_token_ctx(state, token);
		if (state->_depth > 0)
			state->_depth--;
		if (state->_depth > 0)
			state->_expecting_comma[dc(state)] = true;
		return ERROR_RESULT_NO_ERROR;
	}

	if (c == ',') {
		if (!state->_expecting_comma[dc(state)])
			return ERROR_RESULT_JSON_MISSING_COMMA;
		state->_pos++;
		state->_expecting_comma[dc(state)] = false;
		if (state->_in_array[dc(state)]) {
			state->_array_index[dc(state)]++;
			state->_expecting_value[dc(state)] = true;
		} else {
			state->_expecting_key[dc(state)] = true;
		}
		return fun_json_next(state, token);
	}

	if (c == ':') {
		state->_pos++;
		state->_expecting_value[dc(state)] = true;
		return fun_json_next(state, token);
	}

	if (c == ':') {
		state->_pos++;
		state->_expecting_value[dc(state)] = true;
		return fun_json_next(state, token);
	}

	if (state->_in_array[dc(state)] && state->_expecting_value[dc(state)]) {
		uint64_t d = dc(state);
		err = json_value(state, token);
		json_set_token_ctx(state, token);
		state->_expecting_comma[d] = true;
		state->_expecting_value[d] = false;
		return err;
	}

	if (!state->_in_array[dc(state)] && state->_expecting_key[dc(state)]) {
		uint64_t d = dc(state);
		if (c != '"')
			return ERROR_RESULT_JSON_UNEXPECTED_TOKEN;
		err = json_parse_string(state, token, true);
		if (fun_error_is_error(err))
			return err;
		state->_expecting_key[d] = false;
		json_set_token_ctx(state, token);
		return ERROR_RESULT_NO_ERROR;
	}

	if (!state->_in_array[dc(state)] && state->_expecting_value[dc(state)]) {
		uint64_t d = dc(state);
		err = json_value(state, token);
		json_set_token_ctx(state, token);
		state->_expecting_comma[d] = true;
		state->_expecting_value[d] = false;
		state->_expecting_key[d] = true;
		return err;
	}

	return ERROR_RESULT_JSON_UNEXPECTED_TOKEN;
}

ErrorResult fun_json_next_at(FunJsonState *state, uint64_t depth,
							 FunJsonToken *token)
{
	FunJsonToken t;
	ErrorResult err;

	while (1) {
		err = fun_json_next(state, &t);
		if (fun_error_is_error(err))
			return err;
		if (t.type == FUN_JSON_TOKEN_END || t.type == FUN_JSON_INCOMPLETE ||
			t.depth <= depth) {
			*token = t;
			return ERROR_RESULT_NO_ERROR;
		}
	}
}

ErrorResult fun_json_skip_value(FunJsonState *state)
{
	int depth = 0;
	FunJsonToken token;
	ErrorResult err;

	while (1) {
		err = fun_json_next(state, &token);
		if (fun_error_is_error(err))
			return err;
		if (token.type == FUN_JSON_TOKEN_END ||
			token.type == FUN_JSON_INCOMPLETE)
			return ERROR_RESULT_NO_ERROR;
		if (token.type == FUN_JSON_OBJECT_START ||
			token.type == FUN_JSON_ARRAY_START)
			depth++;
		if (token.type == FUN_JSON_OBJECT_END ||
			token.type == FUN_JSON_ARRAY_END) {
			depth--;
			if (depth == 0)
				return ERROR_RESULT_NO_ERROR;
		}
	}
}

ErrorResult fun_json_for_each(String data, uint64_t len, String path,
							  FunJsonEachFn fn, void *context)
{
	if (data == NULL || path == NULL || fn == NULL)
		return ERROR_RESULT_NULL_POINTER;

	FunJsonToken arr;
	ErrorResult err = fun_json_query(data, len, path, &arr);
	if (fun_error_is_error(err))
		return err;
	if (arr.type != FUN_JSON_ARRAY_START)
		return ERROR_RESULT_JSON_TYPE_MISMATCH;

	FunJsonState s;
	char *mdata = (char *)data;
	s._data = mdata;
	s._pos = arr.value ? (uint64_t)(arr.value - data) + 1 : 1;
	s._len = len;
	s._depth = arr.depth;
	s._in_array[arr.depth] = true;
	s._array_index[arr.depth] = 0;
	s._expecting_value[arr.depth] = true;
	s._expecting_key[arr.depth] = false;
	s._expecting_comma[arr.depth] = false;

	uint64_t idx = 0;
	FunJsonToken t;
	int nest = 0;
	while (1) {
		err = fun_json_next_at(&s, arr.depth + 1, &t);
		if (fun_error_is_error(err))
			return err;
		if (t.type == FUN_JSON_TOKEN_END || t.type == FUN_JSON_INCOMPLETE)
			return ERROR_RESULT_NO_ERROR;
		if (nest == 0 && t.type == FUN_JSON_ARRAY_END && t.depth <= arr.depth)
			return ERROR_RESULT_NO_ERROR;
		if (t.type == FUN_JSON_OBJECT_END || t.type == FUN_JSON_ARRAY_END) {
			if (nest > 0)
				nest--;
			continue;
		}
		if (nest == 0 && (t.depth == arr.depth || t.depth == arr.depth + 1) &&
			(t.type == FUN_JSON_OBJECT_START ||
			 t.type == FUN_JSON_ARRAY_START || t.type == FUN_JSON_STRING ||
			 t.type == FUN_JSON_NUMBER || t.type == FUN_JSON_BOOL ||
			 t.type == FUN_JSON_NULL)) {
			err = fn(&t, idx, context, &s);
			if (fun_error_is_error(err))
				return err;
			idx++;
			if (t.type == FUN_JSON_OBJECT_START ||
				t.type == FUN_JSON_ARRAY_START)
				nest++;
		}
	}
	return ERROR_RESULT_NO_ERROR;
}

ErrorResult fun_json_find_key(FunJsonState *state, uint64_t depth, String key,
							  FunJsonToken *token)
{
	FunJsonToken t;
	ErrorResult err;

	while (1) {
		err = fun_json_next(state, &t);
		if (fun_error_is_error(err))
			return err;
		if (t.type == FUN_JSON_TOKEN_END || t.type == FUN_JSON_INCOMPLETE ||
			(t.type == FUN_JSON_OBJECT_END && t.depth <= depth))
			return ERROR_RESULT_JSON_PATH_NOT_FOUND;

		if (t.type == FUN_JSON_KEY && t.depth == depth &&
			t.length == fun_string_length(key) &&
			json_bytes_equal(t.value, t.length, key, fun_string_length(key))) {
			return fun_json_next(state, token);
		}
	}
}

// === Query scan (internal, non-mutating) ===

static ErrorResult json_query_scan(String data, uint64_t len, String path,
								   FunJsonToken *token, bool check_end)
{
	FunJsonState state;
	char *mutable_data = (char *)data;

	state._data = mutable_data;
	state._pos = 0;
	state._len = len;
	state._depth = 0;
	state._in_array[0] = false;
	state._array_index[0] = 0;
	state._expecting_key[0] = false;
	state._expecting_value[0] = false;
	state._expecting_comma[0] = false;

	const char *p = path;
	while (*p) {
		const char *seg_end = p;
		while (*seg_end && *seg_end != '.')
			seg_end++;
		uint64_t seg_len = (uint64_t)(seg_end - p);
		bool is_index = true;
		for (uint64_t i = 0; i < seg_len; i++) {
			if (p[i] < '0' || p[i] > '9') {
				is_index = false;
				break;
			}
		}

		if (state._depth == 0) {
			json_skip_whitespace(&state);
			char first = state._data[state._pos];
			FunJsonToken first_token;
			if (first == '{') {
				json_object(&state, &first_token);
			} else if (first == '[') {
				json_array(&state, &first_token);
			} else {
				return ERROR_RESULT_JSON_PARSE_ERROR;
			}
		}

		FunJsonToken t;
		ErrorResult err;

		if (is_index) {
			uint64_t target_idx = 0;
			for (uint64_t i = 0; i < seg_len; i++)
				target_idx = target_idx * 10 + (uint64_t)(p[i] - '0');

			uint64_t idx = 0;
			while (1) {
				err = fun_json_next(&state, &t);
				if (fun_error_is_error(err))
					return err;
				if (t.type == FUN_JSON_TOKEN_END)
					return ERROR_RESULT_JSON_PATH_NOT_FOUND;
				if (t.type == FUN_JSON_INCOMPLETE)
					return ERROR_RESULT_JSON_INCOMPLETE;
				if ((t.type == FUN_JSON_ARRAY_END && t.depth <= state._depth))
					return ERROR_RESULT_JSON_PATH_NOT_FOUND;
				if (t.depth == state._depth &&
					(t.type == FUN_JSON_OBJECT_START ||
					 t.type == FUN_JSON_ARRAY_START ||
					 t.type == FUN_JSON_STRING || t.type == FUN_JSON_NUMBER ||
					 t.type == FUN_JSON_BOOL || t.type == FUN_JSON_NULL)) {
					if (idx == target_idx)
						goto found_element;
					idx++;
				}
			}
		} else {
			while (1) {
				err = fun_json_next(&state, &t);
				if (fun_error_is_error(err))
					return err;
				if (t.type == FUN_JSON_TOKEN_END ||
					(t.type == FUN_JSON_OBJECT_END && t.depth <= state._depth))
					return ERROR_RESULT_JSON_PATH_NOT_FOUND;
				if (t.type == FUN_JSON_INCOMPLETE)
					return ERROR_RESULT_JSON_INCOMPLETE;
				if (t.type == FUN_JSON_KEY && t.depth == state._depth) {
					if (t.length == seg_len &&
						json_bytes_equal(t.value, t.length, p, seg_len)) {
						goto found_element;
					}
				}
			}
		}

found_element:
		p = *seg_end ? seg_end + 1 : seg_end;
		if (!*p && check_end)
			break;
	}

	return fun_json_next(&state, token);
}

ErrorResult fun_json_query(String data, uint64_t len, String path,
						   FunJsonToken *token)
{
	return json_query_scan(data, len, path, token, true);
}

// === Extractors ===

ErrorResult fun_json_token_copy_string(FunJsonToken *token, OutputString out,
									   uint64_t out_size)
{
	if (token == NULL || out == NULL)
		return ERROR_RESULT_NULL_POINTER;
	if (token->type != FUN_JSON_STRING && token->type != FUN_JSON_KEY)
		return ERROR_RESULT_JSON_TYPE_MISMATCH;
	if (out_size < token->length + 1)
		return ERROR_RESULT_BUFFER_TOO_SMALL;

	for (uint64_t i = 0; i < token->length; i++)
		out[i] = token->value[i];
	out[token->length] = '\0';
	return ERROR_RESULT_NO_ERROR;
}

static int64_tResult json_str_to_int(String s, uint64_t len)
{
	int64_tResult result;
	result.error = ERROR_RESULT_NO_ERROR;
	result.value = 0;

	if (len == 0) {
		result.error = ERROR_RESULT_JSON_TYPE_MISMATCH;
		return result;
	}

	uint64_t i = 0;
	bool neg = false;

	if (s[0] == '-') {
		neg = true;
		i++;
	}

	while (i < len) {
		if (s[i] < '0' || s[i] > '9') {
			result.error = ERROR_RESULT_JSON_TYPE_MISMATCH;
			return result;
		}
		int64_t prev = result.value;
		result.value = result.value * 10 + (int64_t)(s[i] - '0');
		if (result.value < prev) {
			result.error = ERROR_RESULT_JSON_TYPE_MISMATCH;
			return result;
		}
		i++;
	}

	if (neg)
		result.value = -result.value;
	return result;
}

int64_tResult fun_json_token_as_int(FunJsonToken *token)
{
	int64_tResult r;
	r.value = 0;
	r.error = ERROR_RESULT_NO_ERROR;

	if (token == NULL) {
		r.error = ERROR_RESULT_NULL_POINTER;
		return r;
	}
	if (token->type != FUN_JSON_NUMBER) {
		r.error = ERROR_RESULT_JSON_TYPE_MISMATCH;
		return r;
	}
	return json_str_to_int(token->value, token->length);
}

doubleResult fun_json_token_as_double(FunJsonToken *token)
{
	doubleResult r;
	r.value = 0.0;
	r.error = ERROR_RESULT_NO_ERROR;

	if (token == NULL) {
		r.error = ERROR_RESULT_NULL_POINTER;
		return r;
	}
	if (token->type != FUN_JSON_NUMBER) {
		r.error = ERROR_RESULT_JSON_TYPE_MISMATCH;
		return r;
	}

	uint64_t i = 0;
	uint64_t len = token->length;
	String s = token->value;
	double sign = 1.0;

	if (i < len && s[i] == '-') {
		sign = -1.0;
		i++;
	}

	while (i < len && s[i] >= '0' && s[i] <= '9') {
		r.value = r.value * 10.0 + (double)(s[i] - '0');
		i++;
	}

	if (i < len && s[i] == '.') {
		i++;
		double frac = 0.1;
		while (i < len && s[i] >= '0' && s[i] <= '9') {
			r.value += (double)(s[i] - '0') * frac;
			frac *= 0.1;
			i++;
		}
	}

	if (i < len && (s[i] == 'e' || s[i] == 'E')) {
		i++;
		bool exp_neg = false;
		if (i < len && (s[i] == '+' || s[i] == '-')) {
			exp_neg = (s[i] == '-');
			i++;
		}
		double exp = 0.0;
		while (i < len && s[i] >= '0' && s[i] <= '9') {
			exp = exp * 10.0 + (double)(s[i] - '0');
			i++;
		}
		double mult = 1.0;
		for (int j = 0; j < (int)exp; j++)
			mult *= (exp_neg ? 0.1 : 10.0);
		r.value *= mult;
	}

	r.value *= sign;
	return r;
}

boolResult fun_json_token_as_bool(FunJsonToken *token)
{
	boolResult r;
	r.value = false;
	r.error = ERROR_RESULT_NO_ERROR;

	if (token == NULL) {
		r.error = ERROR_RESULT_NULL_POINTER;
		return r;
	}
	if (token->type != FUN_JSON_BOOL) {
		r.error = ERROR_RESULT_JSON_TYPE_MISMATCH;
		return r;
	}
	r.value = true;
	return r;
}

boolResult fun_json_token_is_null(FunJsonToken *token)
{
	boolResult r;
	r.value = false;
	r.error = ERROR_RESULT_NO_ERROR;

	if (token == NULL) {
		r.error = ERROR_RESULT_NULL_POINTER;
		return r;
	}
	r.value = (token->type == FUN_JSON_NULL);
	return r;
}

bool fun_json_token_value_equals(FunJsonToken *token, String expected)
{
	if (token == NULL || expected == NULL)
		return false;
	if (token->value == NULL)
		return false;

	uint64_t elen = fun_string_length(expected);
	if (token->length != elen)
		return false;
	for (uint64_t i = 0; i < elen; i++)
		if (token->value[i] != expected[i])
			return false;
	return true;
}

// === Convenience combinators ===

ErrorResult fun_json_query_string(String data, uint64_t len, String path,
								  OutputString out, uint64_t out_size)
{
	FunJsonToken token;
	ErrorResult err = fun_json_query(data, len, path, &token);
	if (fun_error_is_error(err))
		return err;
	return fun_json_token_copy_string(&token, out, out_size);
}

int64_tResult fun_json_query_int(String data, uint64_t len, String path)
{
	FunJsonToken token;
	int64_tResult r;
	r.value = 0;
	r.error = ERROR_RESULT_NO_ERROR;

	ErrorResult err = fun_json_query(data, len, path, &token);
	if (fun_error_is_error(err)) {
		r.error = err;
		return r;
	}
	return fun_json_token_as_int(&token);
}

doubleResult fun_json_query_double(String data, uint64_t len, String path)
{
	FunJsonToken token;
	doubleResult r;
	r.value = 0.0;
	r.error = ERROR_RESULT_NO_ERROR;

	ErrorResult err = fun_json_query(data, len, path, &token);
	if (fun_error_is_error(err)) {
		r.error = err;
		return r;
	}
	return fun_json_token_as_double(&token);
}

boolResult fun_json_query_bool(String data, uint64_t len, String path)
{
	FunJsonToken token;
	boolResult r;
	r.value = false;
	r.error = ERROR_RESULT_NO_ERROR;

	ErrorResult err = fun_json_query(data, len, path, &token);
	if (fun_error_is_error(err)) {
		r.error = err;
		return r;
	}
	if (token.type != FUN_JSON_BOOL) {
		r.error = ERROR_RESULT_JSON_TYPE_MISMATCH;
		return r;
	}
	r.value = true;
	return r;
}

// === Array extractors ===

static uint64_tResult json_walk_number_array(String data, uint64_t len,
											 String path, void *out,
											 uint64_t max_count, bool is_int)
{
	uint64_tResult r;
	r.value = 0;
	r.error = ERROR_RESULT_NO_ERROR;

	FunJsonToken token;
	ErrorResult err = fun_json_query(data, len, path, &token);
	if (fun_error_is_error(err)) {
		r.error = err;
		return r;
	}
	if (token.type != FUN_JSON_ARRAY_START) {
		r.error = ERROR_RESULT_JSON_TYPE_MISMATCH;
		return r;
	}

	FunJsonState state;
	char *mutable_data = (char *)data;

	state._data = mutable_data;
	state._pos = 0;
	state._len = len;
	state._depth = token.depth;
	state._in_array[state._depth] = true;
	state._array_index[state._depth] = 0;
	state._expecting_value[state._depth] = true;
	state._expecting_comma[state._depth] = false;
	state._expecting_key[state._depth] = false;

	r.value = token.value ? (uint64_t)(token.value - data) + 1 : 1;
	state._pos = r.value;
	r.value = 0;

	while (r.value < max_count) {
		FunJsonToken t;
		err = fun_json_next(&state, &t);
		if (fun_error_is_error(err)) {
			r.error = err;
			return r;
		}
		if (t.type == FUN_JSON_TOKEN_END)
			break;
		if (t.type == FUN_JSON_INCOMPLETE) {
			r.error = ERROR_RESULT_JSON_INCOMPLETE;
			return r;
		}
		if (t.type == FUN_JSON_ARRAY_END && t.depth <= token.depth)
			break;
		if (t.type != FUN_JSON_NUMBER) {
			r.error = ERROR_RESULT_JSON_TYPE_MISMATCH;
			return r;
		}
		if (is_int) {
			int64_tResult ir = fun_json_token_as_int(&t);
			if (fun_error_is_error(ir.error)) {
				r.error = ir.error;
				return r;
			}
			((int64_t *)out)[r.value] = ir.value;
		} else {
			doubleResult dr = fun_json_token_as_double(&t);
			if (fun_error_is_error(dr.error)) {
				r.error = dr.error;
				return r;
			}
			((double *)out)[r.value] = dr.value;
		}
		r.value++;
	}
	return r;
}

uint64_tResult fun_json_query_int_array(String data, uint64_t len, String path,
										int64_t *out, uint64_t max_count)
{
	return json_walk_number_array(data, len, path, (void *)out, max_count,
								  true);
}

uint64_tResult fun_json_query_double_array(String data, uint64_t len,
										   String path, double *out,
										   uint64_t max_count)
{
	return json_walk_number_array(data, len, path, (void *)out, max_count,
								  false);
}

uint64_tResult fun_json_query_string_array(String data, uint64_t len,
										   String path, OutputString buffer,
										   uint64_t buffer_size)
{
	uint64_tResult r;
	r.value = 0;
	r.error = ERROR_RESULT_NO_ERROR;

	FunJsonToken token;
	ErrorResult err = fun_json_query(data, len, path, &token);
	if (fun_error_is_error(err)) {
		r.error = err;
		return r;
	}
	if (token.type != FUN_JSON_ARRAY_START) {
		r.error = ERROR_RESULT_JSON_TYPE_MISMATCH;
		return r;
	}

	FunJsonState state;
	char *mutable_data = (char *)data;

	state._data = mutable_data;
	state._pos = 0;
	state._len = len;
	state._depth = token.depth;
	state._in_array[state._depth] = true;
	state._array_index[state._depth] = 0;
	state._expecting_value[state._depth] = true;
	state._expecting_comma[state._depth] = false;
	state._expecting_key[state._depth] = false;

	uint64_t vp = token.value ? (uint64_t)(token.value - data) + 1 : 1;
	state._pos = vp;

	uint64_t buf_pos = 0;
	while (1) {
		FunJsonToken t;
		err = fun_json_next(&state, &t);
		if (fun_error_is_error(err)) {
			r.error = err;
			return r;
		}
		if (t.type == FUN_JSON_ARRAY_END || t.type == FUN_JSON_TOKEN_END)
			break;
		if (t.type == FUN_JSON_INCOMPLETE) {
			r.error = ERROR_RESULT_JSON_INCOMPLETE;
			return r;
		}
		if (t.type != FUN_JSON_STRING) {
			r.error = ERROR_RESULT_JSON_TYPE_MISMATCH;
			return r;
		}
		if (buf_pos + t.length + 1 > buffer_size)
			break;
		for (uint64_t i = 0; i < t.length; i++)
			buffer[buf_pos + i] = t.value[i];
		buffer[buf_pos + t.length] = '\0';
		buf_pos += t.length + 1;
		r.value++;
	}
	return r;
}

// === init_at_path ===

ErrorResult fun_json_init_at_path(FunJsonState *state, char *data, uint64_t len,
								  String path, uint64_t *base_depth)
{
	if (state == NULL || data == NULL || path == NULL || base_depth == NULL)
		return ERROR_RESULT_NULL_POINTER;

	FunJsonToken token;
	ErrorResult err = json_query_scan(data, len, path, &token, false);
	if (fun_error_is_error(err))
		return err;

	*base_depth = token.depth;

	state->_data = data;
	state->_pos = token.value ? (uint64_t)(token.value - data) : 0;
	state->_len = len;
	state->_depth = token.depth - 1;
	state->_in_array[dc(state)] = false;
	state->_array_index[dc(state)] = 0;
	state->_expecting_key[dc(state)] = false;
	state->_expecting_value[dc(state)] = true;
	state->_expecting_comma[dc(state)] = false;

	return ERROR_RESULT_NO_ERROR;
}
