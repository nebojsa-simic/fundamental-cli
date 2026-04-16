/* Tokenizer test runner using fundamental library */
#include "tokenizer/tokenizer.h"
#include "fundamental/string/string.h"
#include "fundamental/console/console.h"
#include "fundamental/memory/memory.h"
#include "fundamental/filesystem/filesystem.h"
#include "fundamental/async/async.h"
#include "fundamental/file/file.h"

#define GREEN "\x1b[0;32m✓\x1b[0m"

static void print_pass(const char *name)
{
	fun_console_write(GREEN);
	fun_console_write(" ");
	fun_console_write_line(name);
}

/* ─── Helpers ────────────────────────────────────────────────────── */

static RawToken first_token(String src)
{
	Tokenizer t;
	tokenizer_init(&t, src, fun_string_length(src));
	return tokenizer_next(&t);
}

static int tokenize_str(String src, RawTokenArray *out)
{
	Tokenizer t;
	RawTokenArrayResult res = fun_array_RawToken_create(64);
	if (fun_error_is_error(res.error)) {
		return -1;
	}
	*out = res.value;
	tokenizer_init(&t, src, fun_string_length(src));
	return tokenizer_tokenize(&t, out);
}

/* ─── 1a.17: Grammar pattern verification ────────────────────────── */

static int test_word_pattern(void)
{
	RawToken tok = first_token("hello");
	if (tok.type != RAW_WORD || tok.offset != 0 || tok.length != 5 ||
		tok.line != 1 || tok.col != 1) {
		return -1;
	}
	return 0;
}

static int test_word_underscore_start(void)
{
	RawToken tok = first_token("_foo123");
	if (tok.type != RAW_WORD || tok.length != 7) {
		return -1;
	}
	return 0;
}

static int test_number_decimal(void)
{
	RawToken tok = first_token("42");
	if (tok.type != RAW_NUMBER || tok.length != 2) {
		return -1;
	}
	return 0;
}

static int test_number_hex(void)
{
	RawToken tok = first_token("0xFF");
	if (tok.type != RAW_NUMBER || tok.length != 4) {
		return -1;
	}
	return 0;
}

static int test_number_octal(void)
{
	RawToken tok = first_token("0777");
	if (tok.type != RAW_NUMBER || tok.length != 4) {
		return -1;
	}
	return 0;
}

static int test_number_float_dot(void)
{
	RawToken tok = first_token("3.14");
	if (tok.type != RAW_NUMBER || tok.length != 4) {
		return -1;
	}
	return 0;
}

static int test_number_float_exp(void)
{
	RawToken tok = first_token("1e10");
	if (tok.type != RAW_NUMBER || tok.length != 4) {
		return -1;
	}
	return 0;
}

static int test_number_suffix_ul(void)
{
	RawToken tok = first_token("42UL");
	if (tok.type != RAW_NUMBER || tok.length != 4) {
		return -1;
	}
	return 0;
}

static int test_string_simple(void)
{
	RawToken tok = first_token("\"hello\"");
	if (tok.type != RAW_STRING || tok.length != 7) {
		return -1;
	}
	return 0;
}

static int test_char_simple(void)
{
	RawToken tok = first_token("'a'");
	if (tok.type != RAW_CHAR || tok.length != 3) {
		return -1;
	}
	return 0;
}

static int test_hash(void)
{
	RawToken tok = first_token("#");
	if (tok.type != RAW_HASH || tok.length != 1) {
		return -1;
	}
	return 0;
}

static int test_hash_hash(void)
{
	RawToken tok = first_token("##");
	if (tok.type != RAW_HASH_HASH || tok.length != 2) {
		return -1;
	}
	return 0;
}

static int test_ellipsis(void)
{
	RawToken tok = first_token("...");
	if (tok.type != RAW_ELLIPSIS || tok.length != 3) {
		return -1;
	}
	return 0;
}

static int test_multi_char_symbols(void)
{
	typedef struct {
		const char *src;
		RawTokenType expected;
		uint32_t len;
	} Case;
	Case cases[] = {
		{ "<<=", RAW_LSHIFT_ASSIGN, 3 }, { ">>=", RAW_RSHIFT_ASSIGN, 3 },
		{ "+=", RAW_PLUS_ASSIGN, 2 },	 { "-=", RAW_MINUS_ASSIGN, 2 },
		{ "*=", RAW_STAR_ASSIGN, 2 },	 { "/=", RAW_SLASH_ASSIGN, 2 },
		{ "%=", RAW_PERCENT_ASSIGN, 2 }, { "&=", RAW_AMP_ASSIGN, 2 },
		{ "|=", RAW_PIPE_ASSIGN, 2 },	 { "^=", RAW_CARET_ASSIGN, 2 },
		{ "==", RAW_EQ_EQ, 2 },			 { "!=", RAW_BANG_EQ, 2 },
		{ "<=", RAW_LT_EQ, 2 },			 { ">=", RAW_GT_EQ, 2 },
		{ "&&", RAW_AMP_AMP, 2 },		 { "||", RAW_PIPE_PIPE, 2 },
		{ "<<", RAW_LSHIFT, 2 },		 { ">>", RAW_RSHIFT, 2 },
		{ "++", RAW_PLUS_PLUS, 2 },		 { "--", RAW_MINUS_MINUS, 2 },
		{ "->", RAW_ARROW, 2 },
	};
	size_t n = sizeof(cases) / sizeof(cases[0]);
	for (size_t i = 0; i < n; i++) {
		RawToken tok = first_token(cases[i].src);
		if (tok.type != cases[i].expected || tok.length != cases[i].len) {
			return -1;
		}
	}
	return 0;
}

static int test_single_char_symbols(void)
{
	typedef struct {
		char ch;
		RawTokenType expected;
	} Case;
	Case cases[] = {
		{ '+', RAW_PLUS },	 { '-', RAW_MINUS },	 { '*', RAW_STAR },
		{ '/', RAW_SLASH },	 { '%', RAW_PERCENT },	 { '&', RAW_AMPERSAND },
		{ '|', RAW_PIPE },	 { '^', RAW_CARET },	 { '~', RAW_TILDE },
		{ '!', RAW_BANG },	 { '=', RAW_ASSIGN },	 { '<', RAW_LT },
		{ '>', RAW_GT },	 { '.', RAW_DOT },		 { '?', RAW_QUESTION },
		{ ':', RAW_COLON },	 { ';', RAW_SEMICOLON }, { ',', RAW_COMMA },
		{ '(', RAW_LPAREN }, { ')', RAW_RPAREN },	 { '{', RAW_LBRACE },
		{ '}', RAW_RBRACE }, { '[', RAW_LBRACKET },	 { ']', RAW_RBRACKET },
	};
	char buf[2];
	buf[1] = '\0';
	size_t n = sizeof(cases) / sizeof(cases[0]);
	for (size_t i = 0; i < n; i++) {
		buf[0] = cases[i].ch;
		RawToken tok = first_token(buf);
		if (tok.type != cases[i].expected || tok.length != 1) {
			return -1;
		}
	}
	return 0;
}

static int test_eof(void)
{
	RawToken tok = first_token("");
	if (tok.type != RAW_EOF || tok.length != 0) {
		return -1;
	}
	return 0;
}

static int test_error_unexpected_char(void)
{
	RawToken tok = first_token("@");
	if (tok.type != RAW_ERROR || tok.length != 1) {
		return -1;
	}
	return 0;
}

static int test_error_unexpected_backtick(void)
{
	RawToken tok = first_token("`");
	if (tok.type != RAW_ERROR) {
		return -1;
	}
	return 0;
}

static int test_error_unterminated_string(void)
{
	RawToken tok = first_token("\"abc");
	if (tok.type != RAW_ERROR) {
		return -1;
	}
	return 0;
}

static int test_error_unterminated_string_newline(void)
{
	RawToken tok = first_token("\"abc\n\"");
	if (tok.type != RAW_ERROR) {
		return -1;
	}
	return 0;
}

static int test_error_unterminated_char(void)
{
	RawToken tok = first_token("'a");
	if (tok.type != RAW_ERROR) {
		return -1;
	}
	return 0;
}

static int test_error_unterminated_block_comment(void)
{
	RawToken tok = first_token("/* abc");
	if (tok.type != RAW_ERROR) {
		return -1;
	}
	return 0;
}

static int test_grammar_patterns(void)
{
	if (test_word_pattern() != 0)
		return -1;
	if (test_word_underscore_start() != 0)
		return -1;
	if (test_number_decimal() != 0)
		return -1;
	if (test_number_hex() != 0)
		return -1;
	if (test_number_octal() != 0)
		return -1;
	if (test_number_float_dot() != 0)
		return -1;
	if (test_number_float_exp() != 0)
		return -1;
	if (test_number_suffix_ul() != 0)
		return -1;
	if (test_string_simple() != 0)
		return -1;
	if (test_char_simple() != 0)
		return -1;
	if (test_hash() != 0)
		return -1;
	if (test_hash_hash() != 0)
		return -1;
	if (test_ellipsis() != 0)
		return -1;
	if (test_multi_char_symbols() != 0)
		return -1;
	if (test_single_char_symbols() != 0)
		return -1;
	if (test_eof() != 0)
		return -1;
	if (test_error_unexpected_char() != 0)
		return -1;
	if (test_error_unexpected_backtick() != 0)
		return -1;
	if (test_error_unterminated_string() != 0)
		return -1;
	if (test_error_unterminated_string_newline() != 0)
		return -1;
	if (test_error_unterminated_char() != 0)
		return -1;
	if (test_error_unterminated_block_comment() != 0)
		return -1;
	print_pass("1a.17 grammar pattern verification");
	return 0;
}

/* ─── 1a.18: C fragment tests ────────────────────────────────────── */

static int test_fragment_declaration(void)
{
	RawTokenArray arr;
	if (tokenize_str("int x = 42;", &arr) != 0) {
		return -1;
	}

	size_t n = fun_array_RawToken_size(&arr);
	if (n != 6) {
		fun_array_RawToken_destroy(&arr);
		return -1;
	}
	if (fun_array_RawToken_get(&arr, 0).type != RAW_WORD ||
		fun_array_RawToken_get(&arr, 0).length != 3) {
		fun_array_RawToken_destroy(&arr);
		return -1;
	}
	if (fun_array_RawToken_get(&arr, 1).type != RAW_WORD ||
		fun_array_RawToken_get(&arr, 1).length != 1) {
		fun_array_RawToken_destroy(&arr);
		return -1;
	}
	if (fun_array_RawToken_get(&arr, 2).type != RAW_ASSIGN) {
		fun_array_RawToken_destroy(&arr);
		return -1;
	}
	if (fun_array_RawToken_get(&arr, 3).type != RAW_NUMBER ||
		fun_array_RawToken_get(&arr, 3).length != 2) {
		fun_array_RawToken_destroy(&arr);
		return -1;
	}
	if (fun_array_RawToken_get(&arr, 4).type != RAW_SEMICOLON) {
		fun_array_RawToken_destroy(&arr);
		return -1;
	}
	if (fun_array_RawToken_get(&arr, 5).type != RAW_EOF) {
		fun_array_RawToken_destroy(&arr);
		return -1;
	}

	fun_array_RawToken_destroy(&arr);
	return 0;
}

static int test_fragment_arrow(void)
{
	RawTokenArray arr;
	if (tokenize_str("x->y", &arr) != 0) {
		return -1;
	}

	if (fun_array_RawToken_size(&arr) != 4) {
		fun_array_RawToken_destroy(&arr);
		return -1;
	}
	if (fun_array_RawToken_get(&arr, 0).type != RAW_WORD ||
		fun_array_RawToken_get(&arr, 1).type != RAW_ARROW ||
		fun_array_RawToken_get(&arr, 2).type != RAW_WORD ||
		fun_array_RawToken_get(&arr, 3).type != RAW_EOF) {
		fun_array_RawToken_destroy(&arr);
		return -1;
	}

	fun_array_RawToken_destroy(&arr);
	return 0;
}

static int test_fragment_post_increment(void)
{
	RawTokenArray arr;
	if (tokenize_str("x++", &arr) != 0) {
		return -1;
	}

	if (fun_array_RawToken_size(&arr) != 3 ||
		fun_array_RawToken_get(&arr, 1).type != RAW_PLUS_PLUS) {
		fun_array_RawToken_destroy(&arr);
		return -1;
	}

	fun_array_RawToken_destroy(&arr);
	return 0;
}

static int test_fragment_lshift_assign(void)
{
	RawTokenArray arr;
	if (tokenize_str("<<=", &arr) != 0) {
		return -1;
	}

	if (fun_array_RawToken_size(&arr) != 2 ||
		fun_array_RawToken_get(&arr, 0).type != RAW_LSHIFT_ASSIGN ||
		fun_array_RawToken_get(&arr, 0).length != 3) {
		fun_array_RawToken_destroy(&arr);
		return -1;
	}

	fun_array_RawToken_destroy(&arr);
	return 0;
}

static int test_fragment_ellipsis(void)
{
	RawTokenArray arr;
	if (tokenize_str("...", &arr) != 0) {
		return -1;
	}

	if (fun_array_RawToken_size(&arr) != 2 ||
		fun_array_RawToken_get(&arr, 0).type != RAW_ELLIPSIS ||
		fun_array_RawToken_get(&arr, 0).length != 3) {
		fun_array_RawToken_destroy(&arr);
		return -1;
	}

	fun_array_RawToken_destroy(&arr);
	return 0;
}

static int test_fragment_string_with_escape(void)
{
	RawTokenArray arr;
	if (tokenize_str("\"hello\\n\"", &arr) != 0) {
		return -1;
	}

	if (fun_array_RawToken_size(&arr) != 2 ||
		fun_array_RawToken_get(&arr, 0).type != RAW_STRING ||
		fun_array_RawToken_get(&arr, 0).length != 9) {
		fun_array_RawToken_destroy(&arr);
		return -1;
	}

	fun_array_RawToken_destroy(&arr);
	return 0;
}

static int test_fragment_block_comment(void)
{
	RawTokenArray arr;
	if (tokenize_str("a /* comment */ b", &arr) != 0) {
		return -1;
	}

	if (fun_array_RawToken_size(&arr) != 3 ||
		fun_array_RawToken_get(&arr, 0).type != RAW_WORD ||
		fun_array_RawToken_get(&arr, 1).type != RAW_WORD ||
		fun_array_RawToken_get(&arr, 2).type != RAW_EOF) {
		fun_array_RawToken_destroy(&arr);
		return -1;
	}

	fun_array_RawToken_destroy(&arr);
	return 0;
}

static int test_fragment_line_comment(void)
{
	RawTokenArray arr;
	if (tokenize_str("a // comment\nb", &arr) != 0) {
		return -1;
	}

	if (fun_array_RawToken_size(&arr) != 3 ||
		fun_array_RawToken_get(&arr, 0).type != RAW_WORD ||
		fun_array_RawToken_get(&arr, 1).type != RAW_WORD ||
		fun_array_RawToken_get(&arr, 2).type != RAW_EOF) {
		fun_array_RawToken_destroy(&arr);
		return -1;
	}

	fun_array_RawToken_destroy(&arr);
	return 0;
}

static int test_fragment_line_col_tracking(void)
{
	RawTokenArray arr;
	if (tokenize_str("int x = 42;", &arr) != 0) {
		return -1;
	}

	RawToken x_tok = fun_array_RawToken_get(&arr, 1);
	if (x_tok.line != 1 || x_tok.col != 5) {
		fun_array_RawToken_destroy(&arr);
		return -1;
	}

	fun_array_RawToken_destroy(&arr);
	return 0;
}

static int test_fragment_multiline_col(void)
{
	RawTokenArray arr;
	if (tokenize_str("a\nb", &arr) != 0) {
		return -1;
	}

	RawToken a = fun_array_RawToken_get(&arr, 0);
	RawToken b = fun_array_RawToken_get(&arr, 1);
	if (a.line != 1 || a.col != 1 || b.line != 2 || b.col != 1) {
		fun_array_RawToken_destroy(&arr);
		return -1;
	}

	fun_array_RawToken_destroy(&arr);
	return 0;
}

static int test_fragment_lexically_correct_error(void)
{
	RawTokenArray arr;
	if (tokenize_str("a+++++b", &arr) != 0) {
		return -1;
	}

	RawToken a = fun_array_RawToken_get(&arr, 0);
	RawToken a_plus_plus = fun_array_RawToken_get(&arr, 1);
	RawToken plus_plus = fun_array_RawToken_get(&arr, 2);
	RawToken plus = fun_array_RawToken_get(&arr, 3);
	RawToken b_tok = fun_array_RawToken_get(&arr, 4);

	if (a.type != RAW_WORD || a.length != 1 ||
		a_plus_plus.type != RAW_PLUS_PLUS || a_plus_plus.length != 2 ||
		plus_plus.type != RAW_PLUS_PLUS || plus_plus.length != 2 ||
		plus.type != RAW_PLUS || plus.length != 1 || b_tok.type != RAW_WORD ||
		b_tok.length != 1) {
		fun_array_RawToken_destroy(&arr);
		return -1;
	}

	fun_array_RawToken_destroy(&arr);
	return 0;
}

static int test_c_fragments(void)
{
	if (test_fragment_declaration() != 0)
		return -1;
	if (test_fragment_arrow() != 0)
		return -1;
	if (test_fragment_post_increment() != 0)
		return -1;
	if (test_fragment_lshift_assign() != 0)
		return -1;
	if (test_fragment_ellipsis() != 0)
		return -1;
	if (test_fragment_string_with_escape() != 0)
		return -1;
	if (test_fragment_block_comment() != 0)
		return -1;
	if (test_fragment_line_comment() != 0)
		return -1;
	if (test_fragment_line_col_tracking() != 0)
		return -1;
	if (test_fragment_multiline_col() != 0)
		return -1;
	if (test_fragment_lexically_correct_error() != 0)
		return -1;
	print_pass("1a.18 C fragment tokenization");
	return 0;
}

/* ─── 1a.15 + 1a.16: Serialization round-trip ───────────────────── */

static int test_serialize_deserialize(void)
{
	RawTokenArray arr;
	if (tokenize_str("int x = 42;", &arr) != 0) {
		return -1;
	}
	size_t original_count = fun_array_RawToken_size(&arr);

	const char *tmp_path = "test_tmp.tokens";

	voidResult ser = tokenizer_serialize(&arr, tmp_path);
	if (fun_error_is_error(ser.error)) {
		fun_array_RawToken_destroy(&arr);
		return -1;
	}

	RawTokenArrayResult des = tokenizer_deserialize(tmp_path);
	if (fun_error_is_error(des.error) ||
		fun_array_RawToken_size(&des.value) != original_count) {
		fun_array_RawToken_destroy(&arr);
		return -1;
	}

	for (size_t i = 0; i < original_count; i++) {
		RawToken o = fun_array_RawToken_get(&arr, i);
		RawToken r = fun_array_RawToken_get(&des.value, i);
		if (o.type != r.type || o.offset != r.offset || o.length != r.length ||
			o.line != r.line || o.col != r.col) {
			fun_array_RawToken_destroy(&arr);
			fun_array_RawToken_destroy(&des.value);
			return -1;
		}
	}

	fun_array_RawToken_destroy(&arr);
	fun_array_RawToken_destroy(&des.value);
	/* Note: skipping temp file cleanup - no simple delete API in fundamental */

	print_pass("1a.15+1a.16 .tokens serialize/deserialize round-trip");
	return 0;
}

/* ─── 1a.19: Tokenize actual source files ───────────────────────── */

static int test_real_source_files(void)
{
	const char *src_path = "../../src/commands/cmd_version.c";
	Path path = { 0 };
	char path_buf[256];
	fun_path_from_cstr(src_path, path_buf, sizeof(path_buf), &path);

	boolResult exists = fun_file_exists(path);
	if (!exists.value) {
		fun_console_write_line("  (skipping 1a.19: cannot open cmd_version.c)");
		return 0;
	}

	uint64_t file_size = 0;
	voidResult size_res = fun_file_size(path, &file_size);
	if (fun_error_is_error(size_res.error)) {
		fun_console_write_line("  (skipping 1a.19: cannot stat cmd_version.c)");
		return 0;
	}

	MemoryResult buf_res = fun_memory_allocate((size_t)file_size);
	if (fun_error_is_error(buf_res.error)) {
		fun_console_write_line("  (skipping 1a.19: out of memory)");
		return 0;
	}

	Read rp;
	rp.file_path = src_path;
	rp.output = buf_res.value;
	rp.bytes_to_read = file_size;
	rp.offset = 0;
	rp.mode = FILE_MODE_AUTO;
	rp.adaptive = 0;

	AsyncResult async_res = fun_read_file_in_memory(rp);
	voidResult await_res = fun_async_await(&async_res, -1);
	if (fun_error_is_error(await_res.error)) {
		fun_memory_free(&buf_res.value);
		fun_console_write_line("  (skipping 1a.19: cannot read cmd_version.c)");
		return 0;
	}

	char *src_buf = (char *)buf_res.value;

	Tokenizer t;
	tokenizer_init(&t, src_buf, (uint32_t)file_size);

	RawTokenArrayResult res = fun_array_RawToken_create(512);
	if (fun_error_is_error(res.error)) {
		fun_memory_free(&buf_res.value);
		return -1;
	}

	int error_count = tokenizer_tokenize(&t, &res.value);
	size_t tok_count = fun_array_RawToken_size(&res.value);

	if (error_count != 0 || tok_count <= 50 ||
		fun_array_RawToken_get(&res.value, 0).type != RAW_HASH) {
		fun_array_RawToken_destroy(&res.value);
		fun_memory_free(&buf_res.value);
		return -1;
	}

	fun_array_RawToken_destroy(&res.value);
	fun_memory_free(&buf_res.value);

	print_pass("1a.19 tokenize actual source file (cmd_version.c)");
	return 0;
}

/* ─── main ───────────────────────────────────────────────────────── */

int main(void)
{
	int failed = 0;

	fun_console_write_line("Running tokenizer tests:");
	fun_console_write_line("");

	if (test_grammar_patterns() != 0)
		failed++;
	if (test_c_fragments() != 0)
		failed++;
	if (test_serialize_deserialize() != 0)
		failed++;
	if (test_real_source_files() != 0)
		failed++;

	fun_console_write_line("");
	if (failed == 0) {
		fun_console_write_line("All tokenizer tests passed!");
		return 0;
	} else {
		char msg[64];
		StringTemplateParam p[] = { { .key = (String) "n",
									  .value = { .intValue = failed } } };
		fun_string_template((String) "#{n} test(s) failed", p, 1, msg,
							sizeof(msg));
		fun_console_write_line(msg);
		return 1;
	}
}
