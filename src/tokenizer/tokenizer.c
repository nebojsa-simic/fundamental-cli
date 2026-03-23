#include "tokenizer/tokenizer.h"
#include "fundamental/async/async.h"
#include "fundamental/file/file.h"
#include "fundamental/memory/memory.h"

/* ─── Character classification ───────────────────────────────────── */

static int is_word_start(char c)
{
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static int is_word_cont(char c)
{
	return is_word_start(c) || (c >= '0' && c <= '9');
}

static int is_digit(char c)
{
	return c >= '0' && c <= '9';
}

static int is_hex_digit(char c)
{
	return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
		   (c >= 'A' && c <= 'F');
}

static int is_suffix_char(char c)
{
	return c == 'u' || c == 'U' || c == 'l' || c == 'L' || c == 'f' || c == 'F';
}

/* ─── Token construction ──────────────────────────────────────────── */

static RawToken make_token(RawTokenType type, uint32_t offset, uint32_t length,
						   uint32_t line, uint32_t col)
{
	RawToken tok;
	tok.type = type;
	tok.offset = offset;
	tok.length = length;
	tok.line = line;
	tok.col = col;
	return tok;
}

/* ─── Comment skippers ───────────────────────────────────────────── */

/* Skip a // line comment. Cursor must be at the first '/'. */
static void skip_line_comment(Tokenizer *t)
{
	t->offset += 2; /* consume '//' */
	t->col += 2;
	while (t->offset < t->source_len && t->source[t->offset] != '\n' &&
		   t->source[t->offset] != '\r') {
		t->offset++;
		t->col++;
	}
	/* Leave the newline for the whitespace loop to consume */
}

/*
 * Skip a block comment. Cursor must be at the first '/'.
 * Tracks newlines inside the comment for correct line/col after the comment.
 * Returns 0 on success, -1 if input ends before closing "* /".
 */
static int skip_block_comment(Tokenizer *t)
{
	t->offset += 2; /* consume '/ *' */
	t->col += 2;
	while (t->offset < t->source_len) {
		char c = t->source[t->offset];
		if (c == '*' && t->offset + 1 < t->source_len &&
			t->source[t->offset + 1] == '/') {
			t->offset += 2;
			t->col += 2;
			return 0; /* closed */
		}
		if (c == '\n') {
			t->offset++;
			t->line++;
			t->col = 1;
		} else if (c == '\r') {
			t->offset++;
			if (t->offset < t->source_len && t->source[t->offset] == '\n')
				t->offset++;
			t->line++;
			t->col = 1;
		} else {
			t->offset++;
			t->col++;
		}
	}
	return -1; /* unterminated */
}

/* ─── Token scanners ─────────────────────────────────────────────── */

/* 1a.7: WORD tokenization: [a-zA-Z_][a-zA-Z0-9_]* */
static RawToken scan_word(Tokenizer *t, uint32_t start_off, uint32_t line,
						  uint32_t col)
{
	while (t->offset < t->source_len && is_word_cont(t->source[t->offset])) {
		t->offset++;
		t->col++;
	}
	return make_token(RAW_WORD, start_off, t->offset - start_off, line, col);
}

/*
 * 1a.8: NUMBER tokenization.
 * Priority: HEX (0x/0X) → float (with . or e/E) → decimal/octal.
 * Optional suffix: u, U, l, L, f, F (up to 3 chars).
 */
static RawToken scan_number(Tokenizer *t, uint32_t start_off, uint32_t line,
							uint32_t col)
{
	if (t->source[t->offset] == '0' && t->offset + 1 < t->source_len &&
		(t->source[t->offset + 1] == 'x' || t->source[t->offset + 1] == 'X')) {
		/* Hex literal */
		t->offset += 2;
		t->col += 2;
		while (t->offset < t->source_len &&
			   is_hex_digit(t->source[t->offset])) {
			t->offset++;
			t->col++;
		}
	} else {
		/* Decimal, octal, or float */
		while (t->offset < t->source_len && is_digit(t->source[t->offset])) {
			t->offset++;
			t->col++;
		}
		/* Optional decimal point (float) */
		if (t->offset < t->source_len && t->source[t->offset] == '.') {
			t->offset++;
			t->col++;
			while (t->offset < t->source_len &&
				   is_digit(t->source[t->offset])) {
				t->offset++;
				t->col++;
			}
		}
		/* Optional exponent (float) */
		if (t->offset < t->source_len &&
			(t->source[t->offset] == 'e' || t->source[t->offset] == 'E')) {
			t->offset++;
			t->col++;
			if (t->offset < t->source_len &&
				(t->source[t->offset] == '+' || t->source[t->offset] == '-')) {
				t->offset++;
				t->col++;
			}
			while (t->offset < t->source_len &&
				   is_digit(t->source[t->offset])) {
				t->offset++;
				t->col++;
			}
		}
	}
	/* Optional integer/float suffix: u U l L f F (max 3) */
	int n = 0;
	while (n < 3 && t->offset < t->source_len &&
		   is_suffix_char(t->source[t->offset])) {
		t->offset++;
		t->col++;
		n++;
	}
	return make_token(RAW_NUMBER, start_off, t->offset - start_off, line, col);
}

/*
 * 1a.9: STRING tokenization: "..." (raw bytes, escapes tracked but not
 * resolved). Returns RAW_ERROR on unterminated string (bare newline or EOF).
 */
static RawToken scan_string(Tokenizer *t, uint32_t start_off, uint32_t line,
							uint32_t col)
{
	t->offset++; /* consume opening '"' */
	t->col++;
	while (t->offset < t->source_len) {
		char c = t->source[t->offset];
		if (c == '"') {
			t->offset++;
			t->col++;
			return make_token(RAW_STRING, start_off, t->offset - start_off,
							  line, col);
		}
		if (c == '\n' || c == '\r')
			return make_token(RAW_ERROR, start_off, t->offset - start_off, line,
							  col);
		if (c == '\\') {
			/* Escape sequence: skip '\' and the next byte */
			t->offset++;
			t->col++;
			if (t->offset < t->source_len) {
				t->offset++;
				t->col++;
			}
		} else {
			t->offset++;
			t->col++;
		}
	}
	/* EOF without closing '"' */
	return make_token(RAW_ERROR, start_off, t->offset - start_off, line, col);
}

/*
 * 1a.10: CHAR tokenization: '...' (raw bytes, escapes tracked but not
 * resolved). Returns RAW_ERROR on unterminated literal.
 */
static RawToken scan_char(Tokenizer *t, uint32_t start_off, uint32_t line,
						  uint32_t col)
{
	t->offset++; /* consume opening '\'' */
	t->col++;
	while (t->offset < t->source_len) {
		char c = t->source[t->offset];
		if (c == '\'') {
			t->offset++;
			t->col++;
			return make_token(RAW_CHAR, start_off, t->offset - start_off, line,
							  col);
		}
		if (c == '\n' || c == '\r')
			return make_token(RAW_ERROR, start_off, t->offset - start_off, line,
							  col);
		if (c == '\\') {
			t->offset++;
			t->col++;
			if (t->offset < t->source_len) {
				t->offset++;
				t->col++;
			}
		} else {
			t->offset++;
			t->col++;
		}
	}
	return make_token(RAW_ERROR, start_off, t->offset - start_off, line, col);
}

/*
 * 1a.11 + 1a.12 + 1a.13: Symbol tokenization.
 * Longest match: 3-char → 2-char → 1-char.
 * Returns RAW_ERROR for any unrecognised character.
 */
static RawToken scan_symbol(Tokenizer *t, uint32_t start_off, uint32_t line,
							uint32_t col)
{
	uint32_t rem = t->source_len - t->offset;
	char a = t->source[t->offset];
	char b = (rem >= 2) ? t->source[t->offset + 1] : '\0';
	char c = (rem >= 3) ? t->source[t->offset + 2] : '\0';

	/* 3-character tokens */
	if (rem >= 3) {
		if (a == '.' && b == '.' && c == '.') {
			t->offset += 3;
			t->col += 3;
			return make_token(RAW_ELLIPSIS, start_off, 3, line, col);
		}
		if (a == '<' && b == '<' && c == '=') {
			t->offset += 3;
			t->col += 3;
			return make_token(RAW_LSHIFT_ASSIGN, start_off, 3, line, col);
		}
		if (a == '>' && b == '>' && c == '=') {
			t->offset += 3;
			t->col += 3;
			return make_token(RAW_RSHIFT_ASSIGN, start_off, 3, line, col);
		}
	}

/* 2-character tokens */
#define TWO(ch1, ch2, tok_type)                                 \
	if (rem >= 2 && a == (ch1) && b == (ch2)) {                 \
		t->offset += 2;                                         \
		t->col += 2;                                            \
		return make_token((tok_type), start_off, 2, line, col); \
	}

	TWO('+', '=', RAW_PLUS_ASSIGN)
	TWO('-', '=', RAW_MINUS_ASSIGN)
	TWO('*', '=', RAW_STAR_ASSIGN)
	TWO('/', '=', RAW_SLASH_ASSIGN)
	TWO('%', '=', RAW_PERCENT_ASSIGN)
	TWO('&', '=', RAW_AMP_ASSIGN)
	TWO('|', '=', RAW_PIPE_ASSIGN)
	TWO('^', '=', RAW_CARET_ASSIGN)
	TWO('=', '=', RAW_EQ_EQ)
	TWO('!', '=', RAW_BANG_EQ)
	TWO('<', '=', RAW_LT_EQ)
	TWO('>', '=', RAW_GT_EQ)
	TWO('&', '&', RAW_AMP_AMP)
	TWO('|', '|', RAW_PIPE_PIPE)
	TWO('<', '<', RAW_LSHIFT)
	TWO('>', '>', RAW_RSHIFT)
	TWO('+', '+', RAW_PLUS_PLUS)
	TWO('-', '-', RAW_MINUS_MINUS)
	TWO('-', '>', RAW_ARROW)

#undef TWO

	/* 1-character tokens */
	t->offset++;
	t->col++;
	switch (a) {
	case '+':
		return make_token(RAW_PLUS, start_off, 1, line, col);
	case '-':
		return make_token(RAW_MINUS, start_off, 1, line, col);
	case '*':
		return make_token(RAW_STAR, start_off, 1, line, col);
	case '/':
		return make_token(RAW_SLASH, start_off, 1, line, col);
	case '%':
		return make_token(RAW_PERCENT, start_off, 1, line, col);
	case '&':
		return make_token(RAW_AMPERSAND, start_off, 1, line, col);
	case '|':
		return make_token(RAW_PIPE, start_off, 1, line, col);
	case '^':
		return make_token(RAW_CARET, start_off, 1, line, col);
	case '~':
		return make_token(RAW_TILDE, start_off, 1, line, col);
	case '!':
		return make_token(RAW_BANG, start_off, 1, line, col);
	case '=':
		return make_token(RAW_ASSIGN, start_off, 1, line, col);
	case '<':
		return make_token(RAW_LT, start_off, 1, line, col);
	case '>':
		return make_token(RAW_GT, start_off, 1, line, col);
	case '.':
		return make_token(RAW_DOT, start_off, 1, line, col);
	case '?':
		return make_token(RAW_QUESTION, start_off, 1, line, col);
	case ':':
		return make_token(RAW_COLON, start_off, 1, line, col);
	case ';':
		return make_token(RAW_SEMICOLON, start_off, 1, line, col);
	case ',':
		return make_token(RAW_COMMA, start_off, 1, line, col);
	case '(':
		return make_token(RAW_LPAREN, start_off, 1, line, col);
	case ')':
		return make_token(RAW_RPAREN, start_off, 1, line, col);
	case '{':
		return make_token(RAW_LBRACE, start_off, 1, line, col);
	case '}':
		return make_token(RAW_RBRACE, start_off, 1, line, col);
	case '[':
		return make_token(RAW_LBRACKET, start_off, 1, line, col);
	case ']':
		return make_token(RAW_RBRACKET, start_off, 1, line, col);
	default:
		return make_token(RAW_ERROR, start_off, 1, line, col);
	}
}

/* ─── Public API ─────────────────────────────────────────────────── */

/* 1a.4: Tokenizer initialisation */
void tokenizer_init(Tokenizer *t, const char *source, uint32_t length)
{
	t->source = source;
	t->source_len = length;
	t->offset = 0;
	t->line = 1;
	t->col = 1;
}

/* 1a.5–1a.14: tokenizer_next — dispatch based on grammar priority order */
RawToken tokenizer_next(Tokenizer *t)
{
	/* Skip whitespace and comments in a loop (comments may be adjacent) */
	for (;;) {
		/* 1a.5: Whitespace consumption with line/column tracking */
		while (t->offset < t->source_len) {
			char ch = t->source[t->offset];
			if (ch == ' ' || ch == '\t') {
				t->offset++;
				t->col++;
			} else if (ch == '\n') {
				t->offset++;
				t->line++;
				t->col = 1;
			} else if (ch == '\r') {
				t->offset++;
				if (t->offset < t->source_len && t->source[t->offset] == '\n')
					t->offset++;
				t->line++;
				t->col = 1;
			} else {
				break;
			}
		}

		/* 1a.6: Comment skipping */
		if (t->offset + 1 < t->source_len && t->source[t->offset] == '/') {
			if (t->source[t->offset + 1] == '/') {
				skip_line_comment(t);
				continue;
			}
			if (t->source[t->offset + 1] == '*') {
				uint32_t err_off = t->offset;
				uint32_t err_line = t->line;
				uint32_t err_col = t->col;
				if (skip_block_comment(t) != 0) {
					/* 1a.14: Unterminated block comment */
					return make_token(RAW_ERROR, err_off, 2, err_line, err_col);
				}
				continue;
			}
		}
		break;
	}

	/* End of input */
	if (t->offset >= t->source_len)
		return make_token(RAW_EOF, t->offset, 0, t->line, t->col);

	char ch = t->source[t->offset];
	uint32_t start_off = t->offset;
	uint32_t start_line = t->line;
	uint32_t start_col = t->col;

	/* Grammar dispatch priority order (see tokenizer.grammar) */
	if (is_word_start(ch))
		return scan_word(t, start_off, start_line, start_col);
	if (is_digit(ch))
		return scan_number(t, start_off, start_line, start_col);
	if (ch == '"')
		return scan_string(t, start_off, start_line, start_col);
	if (ch == '\'')
		return scan_char(t, start_off, start_line, start_col);
	if (ch == '#') {
		if (t->offset + 1 < t->source_len && t->source[t->offset + 1] == '#') {
			t->offset += 2;
			t->col += 2;
			return make_token(RAW_HASH_HASH, start_off, 2, start_line,
							  start_col);
		}
		t->offset++;
		t->col++;
		return make_token(RAW_HASH, start_off, 1, start_line, start_col);
	}
	/* Multi-char symbols, single-char symbols, or error */
	return scan_symbol(t, start_off, start_line, start_col);
}

int tokenizer_tokenize(Tokenizer *t, RawTokenArray *out)
{
	int error_count = 0;
	for (;;) {
		RawToken tok = tokenizer_next(t);
		ErrorResult push_err = fun_array_RawToken_push(out, tok);
		if (fun_error_is_error(push_err))
			return -1; /* out of memory */
		if (tok.type == RAW_ERROR)
			error_count++;
		if (tok.type == RAW_EOF)
			break;
	}
	return error_count;
}

/* ─── Serialization ──────────────────────────────────────────────── */

#define TOKENS_MAGIC 0x544F4B53u /* 'T','O','K','S' */

/* 1a.15: Serialise RawTokenArray to a binary .tokens file */
voidResult tokenizer_serialize(const RawTokenArray *tokens, const char *path)
{
	voidResult result;
	uint32_t count = (uint32_t)fun_array_RawToken_size(tokens);
	size_t data_size = (size_t)count * sizeof(RawToken);
	size_t total_size = 8 + data_size;

	MemoryResult buf_res = fun_memory_allocate(total_size);
	if (fun_error_is_error(buf_res.error)) {
		result.error = buf_res.error;
		return result;
	}
	Memory buf = buf_res.value;

	/* Write 8-byte header */
	uint32_t *header = (uint32_t *)buf;
	header[0] = TOKENS_MAGIC;
	header[1] = count;

	/* Copy packed token records */
	if (data_size > 0) {
		voidResult cp =
			fun_memory_copy(tokens->array.data, (char *)buf + 8, data_size);
		if (fun_error_is_error(cp.error)) {
			fun_memory_free(&buf);
			result.error = cp.error;
			return result;
		}
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

/* 1a.16: Deserialise a .tokens binary file into a new RawTokenArray */
RawTokenArrayResult tokenizer_deserialize(const char *path)
{
	RawTokenArrayResult result;
	result.error = ERROR_RESULT_NO_ERROR;

	/* Step 1: Read 8-byte header to get magic + count */
	MemoryResult hbuf_res = fun_memory_allocate(8);
	if (fun_error_is_error(hbuf_res.error)) {
		result.error = hbuf_res.error;
		return result;
	}
	Memory hbuf = hbuf_res.value;

	Read rp;
	rp.file_path = path;
	rp.output = hbuf;
	rp.bytes_to_read = 8;
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
	if (hdr[0] != TOKENS_MAGIC) {
		fun_memory_free(&hbuf);
		result.error = fun_error_result(ERROR_CODE_PATH_INVALID,
										"Invalid .tokens file: bad magic");
		return result;
	}
	uint32_t count = hdr[1];
	fun_memory_free(&hbuf);

	/* Create output array */
	size_t cap = count > 0 ? (size_t)count : 1;
	RawTokenArrayResult arr_res = fun_array_RawToken_create(cap);
	if (fun_error_is_error(arr_res.error)) {
		result.error = arr_res.error;
		return result;
	}

	if (count == 0) {
		result.value = arr_res.value;
		return result;
	}

	/* Step 2: Read token records */
	size_t data_size = (size_t)count * sizeof(RawToken);
	MemoryResult dbuf_res = fun_memory_allocate(data_size);
	if (fun_error_is_error(dbuf_res.error)) {
		fun_array_RawToken_destroy(&arr_res.value);
		result.error = dbuf_res.error;
		return result;
	}
	Memory dbuf = dbuf_res.value;

	Read rp2;
	rp2.file_path = path;
	rp2.output = dbuf;
	rp2.bytes_to_read = data_size;
	rp2.offset = 8;
	rp2.mode = FILE_MODE_AUTO;
	rp2.adaptive = 0;

	AsyncResult async2 = fun_read_file_in_memory(rp2);
	voidResult await2 = fun_async_await(&async2, -1);
	if (fun_error_is_error(await2.error)) {
		fun_memory_free(&dbuf);
		fun_array_RawToken_destroy(&arr_res.value);
		result.error = await2.error;
		return result;
	}

	/* Push each token into the array */
	RawToken *raw = (RawToken *)dbuf;
	for (uint32_t i = 0; i < count; i++) {
		ErrorResult push_err = fun_array_RawToken_push(&arr_res.value, raw[i]);
		if (fun_error_is_error(push_err)) {
			fun_memory_free(&dbuf);
			fun_array_RawToken_destroy(&arr_res.value);
			result.error = push_err;
			return result;
		}
	}

	fun_memory_free(&dbuf);
	result.value = arr_res.value;
	return result;
}
