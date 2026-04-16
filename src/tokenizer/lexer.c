#include "tokenizer/lexer.h"
#include "fundamental/async/async.h"
#include "fundamental/file/file.h"
#include "fundamental/memory/memory.h"
#include "fundamental/string/string.h"

/* ─── String table implementation ────────────────────────────────── */

void string_table_init(StringTable *st)
{
	st->data = NULL;
	st->size = 0;
	st->count = 0;
	st->capacity = 0;
}

uint32_t string_table_add(StringTable *st, const char *str, uint32_t len)
{
	if (st->capacity == 0) {
		st->capacity = 256;
		MemoryResult mem_res = fun_memory_allocate(st->capacity);
		if (fun_error_is_error(mem_res.error)) {
			return 0;
		}
		st->data = (char *)mem_res.value;
	}

	uint32_t needed = len + 1;
	if (st->size + needed > st->capacity) {
		uint32_t new_cap = st->capacity * 2;
		while (new_cap < st->size + needed) {
			new_cap *= 2;
		}
		MemoryResult mem_res = fun_memory_allocate(new_cap);
		if (fun_error_is_error(mem_res.error)) {
			return 0;
		}
		if (st->data) {
			fun_memory_copy(st->data, mem_res.value, st->size);
			fun_memory_free((Memory *)&st->data);
		}
		st->data = (char *)mem_res.value;
		st->capacity = new_cap;
	}

	uint32_t offset = st->size;
	fun_memory_copy((void *)str, st->data + offset, len);
	st->data[offset + len] = '\0';
	st->size += needed;
	st->count++;
	return offset;
}

const char *string_table_get(const StringTable *st, uint32_t offset)
{
	if (offset >= st->size) {
		return "";
	}
	return st->data + offset;
}

void string_table_destroy(StringTable *st)
{
	if (st->data) {
		Memory mem = st->data;
		fun_memory_free(&mem);
		st->data = NULL;
	}
	st->size = 0;
	st->count = 0;
	st->capacity = 0;
}

/* ─── Character classification for escape sequences ─────────────── */

static int hex_digit_value(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return 10 + (c - 'a');
	if (c >= 'A' && c <= 'F')
		return 10 + (c - 'A');
	return -1;
}

static int octal_digit_value(char c)
{
	if (c >= '0' && c <= '7')
		return c - '0';
	return -1;
}

/* ─── Escape sequence resolution ─────────────────────────────────── */

static int resolve_escape_sequence(const char *src, uint32_t len,
								   uint64_t *out_value,
								   uint32_t *chars_consumed)
{
	if (len < 2 || src[0] != '\\') {
		*chars_consumed = 1;
		*out_value = (uint8_t)src[0];
		return 0;
	}

	char esc = src[1];
	switch (esc) {
	case 'a':
		*out_value = '\a';
		*chars_consumed = 2;
		return 0;
	case 'b':
		*out_value = '\b';
		*chars_consumed = 2;
		return 0;
	case 'f':
		*out_value = '\f';
		*chars_consumed = 2;
		return 0;
	case 'n':
		*out_value = '\n';
		*chars_consumed = 2;
		return 0;
	case 'r':
		*out_value = '\r';
		*chars_consumed = 2;
		return 0;
	case 't':
		*out_value = '\t';
		*chars_consumed = 2;
		return 0;
	case 'v':
		*out_value = '\v';
		*chars_consumed = 2;
		return 0;
	case '\\':
		*out_value = '\\';
		*chars_consumed = 2;
		return 0;
	case '\'':
		*out_value = '\'';
		*chars_consumed = 2;
		return 0;
	case '\"':
		*out_value = '\"';
		*chars_consumed = 2;
		return 0;
	case '?':
		*out_value = '?';
		*chars_consumed = 2;
		return 0;
	case 'x': {
		if (len < 4) {
			*chars_consumed = 1;
			*out_value = '\\';
			return -1;
		}
		int v1 = hex_digit_value(src[2]);
		int v2 = hex_digit_value(src[3]);
		if (v1 < 0 || v2 < 0) {
			*chars_consumed = 1;
			*out_value = '\\';
			return -1;
		}
		*out_value = (uint8_t)((v1 << 4) | v2);
		*chars_consumed = 4;
		return 0;
	}
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7': {
		uint32_t consumed = 1;
		uint64_t value = octal_digit_value(src[1]);
		while (consumed < 3 && len > 1 + consumed) {
			int v = octal_digit_value(src[1 + consumed]);
			if (v < 0)
				break;
			value = (value << 3) | v;
			consumed++;
		}
		*out_value = (uint8_t)value;
		*chars_consumed = 1 + consumed;
		return 0;
	}
	default:
		*chars_consumed = 1;
		*out_value = '\\';
		return -1;
	}
}

/* ─── Keyword lookup ─────────────────────────────────────────────── */

static LexTokenType classify_keyword(const char *word, uint32_t len)
{
#define KEYWORD_CHECK(kw, tok)               \
	if (len == sizeof(kw) - 1) {             \
		int match = 1;                       \
		for (uint32_t i = 0; i < len; i++) { \
			if (word[i] != kw[i]) {          \
				match = 0;                   \
				break;                       \
			}                                \
		}                                    \
		if (match)                           \
			return tok;                      \
	}

	/* Type keywords */
	KEYWORD_CHECK("int", LEX_INT)
	KEYWORD_CHECK("char", LEX_CHAR)
	KEYWORD_CHECK("void", LEX_VOID)
	KEYWORD_CHECK("float", LEX_FLOAT)
	KEYWORD_CHECK("double", LEX_DOUBLE)
	KEYWORD_CHECK("struct", LEX_STRUCT)
	KEYWORD_CHECK("union", LEX_UNION)
	KEYWORD_CHECK("enum", LEX_ENUM)
	KEYWORD_CHECK("typedef", LEX_TYPEDEF)
	KEYWORD_CHECK("signed", LEX_SIGNED)
	KEYWORD_CHECK("unsigned", LEX_UNSIGNED)
	KEYWORD_CHECK("short", LEX_SHORT)
	KEYWORD_CHECK("long", LEX_LONG)

	/* Control flow */
	KEYWORD_CHECK("if", LEX_IF)
	KEYWORD_CHECK("else", LEX_ELSE)
	KEYWORD_CHECK("for", LEX_FOR)
	KEYWORD_CHECK("while", LEX_WHILE)
	KEYWORD_CHECK("do", LEX_DO)
	KEYWORD_CHECK("switch", LEX_SWITCH)
	KEYWORD_CHECK("case", LEX_CASE)
	KEYWORD_CHECK("default", LEX_DEFAULT)
	KEYWORD_CHECK("break", LEX_BREAK)
	KEYWORD_CHECK("continue", LEX_CONTINUE)
	KEYWORD_CHECK("return", LEX_RETURN)
	KEYWORD_CHECK("goto", LEX_GOTO)

	/* Storage class and qualifiers */
	KEYWORD_CHECK("static", LEX_STATIC)
	KEYWORD_CHECK("const", LEX_CONST)
	KEYWORD_CHECK("inline", LEX_INLINE)
	KEYWORD_CHECK("extern", LEX_EXTERN)
	KEYWORD_CHECK("volatile", LEX_VOLATILE)
	KEYWORD_CHECK("restrict", LEX_RESTRICT)

	/* Operators */
	KEYWORD_CHECK("sizeof", LEX_SIZEOF)
	KEYWORD_CHECK("_Alignof", LEX_ALIGNOF)
	KEYWORD_CHECK("_Generic", LEX__GENERIC)

#undef KEYWORD_CHECK

	return LEX_IDENTIFIER;
}

/* ─── LexToken construction ─────────────────────────────────────── */

static LexToken make_lex_token(LexTokenType type, uint32_t offset,
							   uint32_t length, uint32_t line, uint32_t col,
							   uint64_t value)
{
	LexToken tok;
	tok.type = type;
	tok.offset = offset;
	tok.length = length;
	tok.line = line;
	tok.col = col;
	tok.value = value;
	return tok;
}

/* ─── Literal parsing ────────────────────────────────────────────── */

static uint64_t parse_integer_literal(const char *text, uint32_t len,
									  int *is_error)
{
	*is_error = 0;

	if (len == 0) {
		*is_error = 1;
		return 0;
	}

	/* Check for hex prefix */
	if (len >= 2 && text[0] == '0' && (text[1] == 'x' || text[1] == 'X')) {
		uint64_t result = 0;
		for (uint32_t i = 2; i < len; i++) {
			char c = text[i];
			if (c >= '0' && c <= '9') {
				result = (result << 4) | (c - '0');
			} else if (c >= 'a' && c <= 'f') {
				result = (result << 4) | (10 + (c - 'a'));
			} else if (c >= 'A' && c <= 'F') {
				result = (result << 4) | (10 + (c - 'A'));
			} else if (c == 'u' || c == 'U' || c == 'l' || c == 'L' ||
					   c == 'f' || c == 'F') {
				break; /* Suffix */
			} else {
				*is_error = 1;
				return 0;
			}
		}
		return result;
	}

	/* Decimal or octal */
	uint64_t result = 0;
	int base = (text[0] == '0' && len > 1) ? 8 : 10;

	for (uint32_t i = 0; i < len; i++) {
		char c = text[i];
		if (c >= '0' && c <= '7') {
			result = result * base + (c - '0');
		} else if (base == 10 && (c == '8' || c == '9')) {
			result = result * 10 + (c - '0');
		} else if (c == 'u' || c == 'U' || c == 'l' || c == 'L' || c == 'f' ||
				   c == 'F') {
			break; /* Suffix */
		} else if (c == '.' || c == 'e' || c == 'E') {
			break; /* Float literal */
		} else {
			*is_error = 1;
			return 0;
		}
	}

	return result;
}

static uint64_t parse_float_literal(const char *text, uint32_t len,
									int *is_error)
{
	*is_error = 0;

	/* Simple implementation: find decimal point, parse integer and fractional parts */
	uint64_t int_part = 0;
	uint64_t frac_part = 0;
	int frac_digits = 0;
	int in_frac = 0;
	int has_point = 0;

	uint32_t i = 0;

	/* Skip leading zeros */
	while (i < len && text[i] == '0')
		i++;

	/* Parse integer part */
	while (i < len) {
		char c = text[i];
		if (c >= '0' && c <= '9') {
			if (!in_frac) {
				int_part = int_part * 10 + (c - '0');
			} else {
				frac_part = frac_part * 10 + (c - '0');
				frac_digits++;
			}
		} else if (c == '.' && !has_point) {
			has_point = 1;
			in_frac = 1;
		} else if (c == 'e' || c == 'E') {
			/* Exponent - simplified handling */
			break;
		} else if (c == 'u' || c == 'U' || c == 'l' || c == 'L' || c == 'f' ||
				   c == 'F') {
			break;
		} else {
			*is_error = 1;
			return 0;
		}
		i++;
	}

	/* Convert to double representation stored as uint64_t */
	/* For now, return a simple representation - a real implementation would use proper IEEE 754 */
	(void)frac_part;
	(void)frac_digits;

	/* Placeholder: in a real implementation, this would compute the actual IEEE 754 representation */
	*is_error = 0;
	return 0; /* Placeholder */
}

static int resolve_char_literal(const char *text, uint32_t len,
								uint64_t *out_value)
{
	if (len < 2) { /* Must have at least opening and closing quote */
		return -1;
	}

	if (text[0] != '\'' || text[len - 1] != '\'') {
		return -1;
	}

	if (len == 2) { /* Empty: '' */
		return -1;
	}

	if (len == 3) { /* Simple: 'a' */
		*out_value = (uint8_t)text[1];
		return 0;
	}

	if (text[1] == '\\') { /* Escape sequence */
		uint32_t consumed = 0;
		return resolve_escape_sequence(text + 1, len - 2, out_value, &consumed);
	}

	/* Multi-character char literal (implementation-defined) */
	*out_value = 0;
	for (uint32_t i = 1; i < len - 1; i++) {
		*out_value = (*out_value << 8) | (uint8_t)text[i];
	}
	return 0;
}

static int resolve_string_literal(const char *text, uint32_t len,
								  StringTable *st, uint32_t *out_offset)
{
	if (len < 2 || text[0] != '"' || text[len - 1] != '"') {
		return -1;
	}

	/* Allocate buffer for resolved string */
	MemoryResult buf_res = fun_memory_allocate(len);
	if (fun_error_is_error(buf_res.error)) {
		return -1;
	}
	char *buf = (char *)buf_res.value;

	uint32_t out_idx = 0;
	uint32_t i = 1; /* Skip opening quote */

	while (i < len - 1) { /* Stop before closing quote */
		if (text[i] == '\\' && i + 1 < len - 1) {
			uint32_t consumed = 0;
			uint64_t value = 0;
			int err = resolve_escape_sequence(text + i, len - i - 1, &value,
											  &consumed);
			(void)err; /* Ignore errors for now */
			buf[out_idx++] = (char)value;
			i += consumed;
		} else {
			buf[out_idx++] = text[i];
			i++;
		}
	}

	buf[out_idx] = '\0';
	*out_offset = string_table_add(st, buf, out_idx);
	fun_memory_free(&buf_res.value);
	return 0;
}

/* ─── Operator mapping ───────────────────────────────────────────── */

static LexTokenType map_raw_symbol_to_lex(RawTokenType raw_type)
{
	switch (raw_type) {
	case RAW_HASH:
		return LEX_HASH;
	case RAW_HASH_HASH:
		return LEX_HASH_HASH;
	case RAW_ELLIPSIS:
		return LEX_ELLIPSIS;
	case RAW_LSHIFT_ASSIGN:
		return LEX_LSHIFT_ASSIGN;
	case RAW_RSHIFT_ASSIGN:
		return LEX_RSHIFT_ASSIGN;
	case RAW_PLUS_ASSIGN:
		return LEX_PLUS_ASSIGN;
	case RAW_MINUS_ASSIGN:
		return LEX_MINUS_ASSIGN;
	case RAW_STAR_ASSIGN:
		return LEX_STAR_ASSIGN;
	case RAW_SLASH_ASSIGN:
		return LEX_SLASH_ASSIGN;
	case RAW_PERCENT_ASSIGN:
		return LEX_PERCENT_ASSIGN;
	case RAW_AMP_ASSIGN:
		return LEX_AMP_ASSIGN;
	case RAW_PIPE_ASSIGN:
		return LEX_PIPE_ASSIGN;
	case RAW_CARET_ASSIGN:
		return LEX_CARET_ASSIGN;
	case RAW_EQ_EQ:
		return LEX_EQ;
	case RAW_BANG_EQ:
		return LEX_NEQ;
	case RAW_LT_EQ:
		return LEX_LTE;
	case RAW_GT_EQ:
		return LEX_GTE;
	case RAW_AMP_AMP:
		return LEX_AMP_AMP;
	case RAW_PIPE_PIPE:
		return LEX_PIPE_PIPE;
	case RAW_LSHIFT:
		return LEX_LSHIFT;
	case RAW_RSHIFT:
		return LEX_RSHIFT;
	case RAW_PLUS_PLUS:
		return LEX_INCREMENT;
	case RAW_MINUS_MINUS:
		return LEX_DECREMENT;
	case RAW_ARROW:
		return LEX_ARROW;
	case RAW_PLUS:
		return LEX_PLUS;
	case RAW_MINUS:
		return LEX_MINUS;
	case RAW_STAR:
		return LEX_STAR;
	case RAW_SLASH:
		return LEX_SLASH;
	case RAW_PERCENT:
		return LEX_PERCENT;
	case RAW_AMPERSAND:
		return LEX_AMPERSAND;
	case RAW_PIPE:
		return LEX_PIPE;
	case RAW_CARET:
		return LEX_CARET;
	case RAW_TILDE:
		return LEX_TILDE;
	case RAW_BANG:
		return LEX_BANG;
	case RAW_ASSIGN:
		return LEX_ASSIGN;
	case RAW_LT:
		return LEX_LT;
	case RAW_GT:
		return LEX_GT;
	case RAW_DOT:
		return LEX_DOT;
	case RAW_QUESTION:
		return LEX_QUESTION;
	case RAW_COLON:
		return LEX_COLON;
	case RAW_SEMICOLON:
		return LEX_SEMICOLON;
	case RAW_COMMA:
		return LEX_COMMA;
	case RAW_LPAREN:
		return LEX_LPAREN;
	case RAW_RPAREN:
		return LEX_RPAREN;
	case RAW_LBRACE:
		return LEX_LBRACE;
	case RAW_RBRACE:
		return LEX_RBRACE;
	case RAW_LBRACKET:
		return LEX_LBRACKET;
	case RAW_RBRACKET:
		return LEX_RBRACKET;
	default:
		return LEX_ERROR;
	}
}

/* ─── Public API ─────────────────────────────────────────────────── */

void lexer_init(Lexer *l, const RawToken *raw_tokens, uint32_t raw_count,
				const char *source, uint32_t source_len)
{
	l->raw_tokens = raw_tokens;
	l->raw_count = raw_count;
	l->index = 0;
	l->source = source;
	l->source_len = source_len;
	string_table_init(&l->string_table);
}

LexToken lexer_next(Lexer *l)
{
	if (l->index >= l->raw_count) {
		return make_lex_token(LEX_EOF, 0, 0, 0, 0, 0);
	}

	const RawToken *raw = &l->raw_tokens[l->index];
	l->index++;

	switch (raw->type) {
	case RAW_WORD: {
		const char *word = l->source + raw->offset;
		LexTokenType keyword_type = classify_keyword(word, raw->length);
		return make_lex_token(keyword_type, raw->offset, raw->length, raw->line,
							  raw->col, 0);
	}

	case RAW_NUMBER: {
		const char *num_text = l->source + raw->offset;
		int is_error = 0;

		/* Check if it's a float literal */
		int is_float = 0;
		int is_hex = (raw->length >= 2 && num_text[0] == '0' &&
					  (num_text[1] == 'x' || num_text[1] == 'X'));
		for (uint32_t i = 0; i < raw->length; i++) {
			char c = num_text[i];
			if (c == '.') {
				is_float = 1;
				break;
			}
			/* 'e' or 'E' only indicates float in decimal numbers, not hex */
			if (!is_hex && (c == 'e' || c == 'E')) {
				is_float = 1;
				break;
			}
		}

		uint64_t value;
		if (is_float) {
			value = parse_float_literal(num_text, raw->length, &is_error);
			return make_lex_token(LEX_FLOAT_LITERAL, raw->offset, raw->length,
								  raw->line, raw->col, value);
		} else {
			value = parse_integer_literal(num_text, raw->length, &is_error);
			return make_lex_token(LEX_INT_LITERAL, raw->offset, raw->length,
								  raw->line, raw->col, value);
		}
	}

	case RAW_STRING: {
		uint32_t str_offset = 0;
		const char *str_text = l->source + raw->offset;
		int err = resolve_string_literal(str_text, raw->length,
										 &l->string_table, &str_offset);
		if (err != 0) {
			return make_lex_token(LEX_ERROR, raw->offset, raw->length,
								  raw->line, raw->col, 0);
		}
		return make_lex_token(LEX_STRING_LITERAL, raw->offset, raw->length,
							  raw->line, raw->col, str_offset);
	}

	case RAW_CHAR: {
		const char *char_text = l->source + raw->offset;
		uint64_t value = 0;
		int err = resolve_char_literal(char_text, raw->length, &value);
		if (err != 0) {
			return make_lex_token(LEX_ERROR, raw->offset, raw->length,
								  raw->line, raw->col, 0);
		}
		return make_lex_token(LEX_CHAR_LITERAL, raw->offset, raw->length,
							  raw->line, raw->col, value);
	}

	case RAW_EOF:
		return make_lex_token(LEX_EOF, raw->offset, 0, raw->line, raw->col, 0);

	case RAW_ERROR:
		return make_lex_token(LEX_ERROR, raw->offset, raw->length, raw->line,
							  raw->col, 0);

	default: {
		LexTokenType lex_type = map_raw_symbol_to_lex(raw->type);
		return make_lex_token(lex_type, raw->offset, raw->length, raw->line,
							  raw->col, 0);
	}
	}
}

int lexer_lex(Lexer *l, LexTokenArray *out)
{
	int error_count = 0;
	for (;;) {
		LexToken tok = lexer_next(l);
		ErrorResult push_err = fun_array_LexToken_push(out, tok);
		if (fun_error_is_error(push_err)) {
			return -1;
		}
		if (tok.type == LEX_ERROR) {
			error_count++;
		}
		if (tok.type == LEX_EOF) {
			break;
		}
	}
	return error_count;
}

/* ─── Serialization ──────────────────────────────────────────────── */

#define LEX_MAGIC 0x4C455846u /* 'L','E','X','F' */

voidResult lexer_serialize(const LexTokenArray *tokens, const StringTable *st,
						   const char *path)
{
	voidResult result;

	uint32_t count = (uint32_t)fun_array_LexToken_size(tokens);
	size_t tokens_size = (size_t)count * sizeof(LexToken);
	size_t total_size = 12 + tokens_size + st->size;

	MemoryResult buf_res = fun_memory_allocate(total_size);
	if (fun_error_is_error(buf_res.error)) {
		result.error = buf_res.error;
		return result;
	}
	Memory buf = buf_res.value;

	char *buf_ptr = (char *)buf;

	/* Write header */
	*(uint32_t *)buf_ptr = LEX_MAGIC;
	*(uint32_t *)(buf_ptr + 4) = count;
	*(uint32_t *)(buf_ptr + 8) = st->size;
	buf_ptr += 12;

	/* Copy tokens */
	if (tokens_size > 0) {
		fun_memory_copy(tokens->array.data, buf_ptr, tokens_size);
		buf_ptr += tokens_size;
	}

	/* Copy string table */
	if (st->size > 0 && st->data) {
		fun_memory_copy(st->data, buf_ptr, st->size);
	}

	Write wp;
	wp.file_path = path;
	wp.input = buf;
	wp.bytes_to_write = total_size;
	wp.offset = 0;
	wp.mode = FILE_MODE_AUTO;
	wp.adaptive = 0;

	AsyncResult async = fun_write_memory_to_file(wp);
	voidResult await_res = fun_async_await(&async, -1);
	fun_memory_free(&buf);

	return await_res;
}

LexResult lexer_deserialize(const char *path)
{
	LexResult result;
	result.error = ERROR_RESULT_NO_ERROR;

	/* Read header */
	MemoryResult hbuf_res = fun_memory_allocate(12);
	if (fun_error_is_error(hbuf_res.error)) {
		result.error = hbuf_res.error;
		return result;
	}
	Memory hbuf = hbuf_res.value;

	Read rp;
	rp.file_path = path;
	rp.output = hbuf;
	rp.bytes_to_read = 12;
	rp.offset = 0;
	rp.mode = FILE_MODE_AUTO;
	rp.adaptive = 0;

	AsyncResult async1 = fun_read_file_in_memory(rp);
	voidResult await1 = fun_async_await(&async1, -1);
	if (fun_error_is_error(await1.error)) {
		fun_memory_free(&hbuf);
		result.error = await1.error;
		return result;
	}

	uint32_t *hdr = (uint32_t *)hbuf;
	if (hdr[0] != LEX_MAGIC) {
		fun_memory_free(&hbuf);
		result.error = fun_error_result(ERROR_CODE_PATH_INVALID,
										"Invalid .lex file: bad magic");
		return result;
	}

	uint32_t count = hdr[1];
	uint32_t st_size = hdr[2];
	fun_memory_free(&hbuf);

	/* Create tokens array */
	size_t cap = count > 0 ? (size_t)count : 1;
	LexTokenArrayResult arr_res = fun_array_LexToken_create(cap);
	if (fun_error_is_error(arr_res.error)) {
		result.error = arr_res.error;
		return result;
	}
	result.tokens = arr_res.value;

	/* Read tokens */
	if (count > 0) {
		size_t tokens_size = (size_t)count * sizeof(LexToken);
		MemoryResult tbuf_res = fun_memory_allocate(tokens_size);
		if (fun_error_is_error(tbuf_res.error)) {
			fun_array_LexToken_destroy(&result.tokens);
			result.error = tbuf_res.error;
			return result;
		}
		Memory tbuf = tbuf_res.value;

		Read rp2;
		rp2.file_path = path;
		rp2.output = tbuf;
		rp2.bytes_to_read = tokens_size;
		rp2.offset = 12;
		rp2.mode = FILE_MODE_AUTO;
		rp2.adaptive = 0;

		AsyncResult async2 = fun_read_file_in_memory(rp2);
		voidResult await2 = fun_async_await(&async2, -1);
		if (fun_error_is_error(await2.error)) {
			fun_memory_free(&tbuf);
			fun_array_LexToken_destroy(&result.tokens);
			result.error = await2.error;
			return result;
		}

		LexToken *raw_tokens = (LexToken *)tbuf;
		for (uint32_t i = 0; i < count; i++) {
			ErrorResult push_err =
				fun_array_LexToken_push(&result.tokens, raw_tokens[i]);
			if (fun_error_is_error(push_err)) {
				fun_memory_free(&tbuf);
				fun_array_LexToken_destroy(&result.tokens);
				result.error = push_err;
				return result;
			}
		}
		fun_memory_free(&tbuf);
	}

	/* Read string table */
	string_table_init(&result.string_table);
	if (st_size > 0) {
		MemoryResult stbuf_res = fun_memory_allocate(st_size);
		if (fun_error_is_error(stbuf_res.error)) {
			fun_array_LexToken_destroy(&result.tokens);
			result.error = stbuf_res.error;
			return result;
		}
		Memory stbuf = stbuf_res.value;

		Read rp3;
		rp3.file_path = path;
		rp3.output = stbuf;
		rp3.bytes_to_read = st_size;
		rp3.offset = 12 + count * sizeof(LexToken);
		rp3.mode = FILE_MODE_AUTO;
		rp3.adaptive = 0;

		AsyncResult async3 = fun_read_file_in_memory(rp3);
		voidResult await3 = fun_async_await(&async3, -1);
		if (fun_error_is_error(await3.error)) {
			fun_memory_free(&stbuf);
			fun_array_LexToken_destroy(&result.tokens);
			result.error = await3.error;
			return result;
		}

		result.string_table.data = (char *)stbuf;
		result.string_table.size = st_size;
		result.string_table.capacity = st_size;
	}

	return result;
}
