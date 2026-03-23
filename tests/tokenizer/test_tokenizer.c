#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "fundamental/string/string.h"
#include "fundamental/filesystem/path.h"
#include "tokenizer/tokenizer.h"

#define GREEN "\033[0;32m\u2713\033[0m"

static void print_pass(const char *name)
{
	printf("%s %s\n", GREEN, name);
}

/* ─── Helpers ────────────────────────────────────────────────────── */

static RawToken first_token(String src)
{
	Tokenizer t;
	tokenizer_init(&t, src, fun_string_length(src));
	return tokenizer_next(&t);
}

static void tokenize_str(String src, RawTokenArray *out)
{
	Tokenizer t;
	RawTokenArrayResult res = fun_array_RawToken_create(64);
	assert(res.error.code == 0);
	*out = res.value;
	tokenizer_init(&t, src, fun_string_length(src));
	tokenizer_tokenize(&t, out);
}

/* ─── 1a.17: Grammar pattern verification ────────────────────────── */

static void test_word_pattern(void)
{
	RawToken tok = first_token("hello");
	assert(tok.type == RAW_WORD);
	assert(tok.offset == 0);
	assert(tok.length == 5);
	assert(tok.line == 1);
	assert(tok.col == 1);
}

static void test_word_underscore_start(void)
{
	RawToken tok = first_token("_foo123");
	assert(tok.type == RAW_WORD);
	assert(tok.length == 7);
}

static void test_number_decimal(void)
{
	RawToken tok = first_token("42");
	assert(tok.type == RAW_NUMBER);
	assert(tok.length == 2);
}

static void test_number_hex(void)
{
	RawToken tok = first_token("0xFF");
	assert(tok.type == RAW_NUMBER);
	assert(tok.length == 4);
}

static void test_number_octal(void)
{
	RawToken tok = first_token("0777");
	assert(tok.type == RAW_NUMBER);
	assert(tok.length == 4);
}

static void test_number_float_dot(void)
{
	RawToken tok = first_token("3.14");
	assert(tok.type == RAW_NUMBER);
	assert(tok.length == 4);
}

static void test_number_float_exp(void)
{
	RawToken tok = first_token("1e10");
	assert(tok.type == RAW_NUMBER);
	assert(tok.length == 4);
}

static void test_number_suffix_ul(void)
{
	RawToken tok = first_token("42UL");
	assert(tok.type == RAW_NUMBER);
	assert(tok.length == 4);
}

static void test_string_simple(void)
{
	RawToken tok = first_token("\"hello\"");
	assert(tok.type == RAW_STRING);
	assert(tok.length == 7);
}

static void test_char_simple(void)
{
	RawToken tok = first_token("'a'");
	assert(tok.type == RAW_CHAR);
	assert(tok.length == 3);
}

static void test_hash(void)
{
	RawToken tok = first_token("#");
	assert(tok.type == RAW_HASH);
	assert(tok.length == 1);
}

static void test_hash_hash(void)
{
	RawToken tok = first_token("##");
	assert(tok.type == RAW_HASH_HASH);
	assert(tok.length == 2);
}

static void test_ellipsis(void)
{
	RawToken tok = first_token("...");
	assert(tok.type == RAW_ELLIPSIS);
	assert(tok.length == 3);
}

static void test_multi_char_symbols(void)
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
		assert(tok.type == cases[i].expected);
		assert(tok.length == cases[i].len);
	}
}

static void test_single_char_symbols(void)
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
		assert(tok.type == cases[i].expected);
		assert(tok.length == 1);
	}
}

static void test_eof(void)
{
	RawToken tok = first_token("");
	assert(tok.type == RAW_EOF);
	assert(tok.length == 0);
}

static void test_error_unexpected_char(void)
{
	RawToken tok = first_token("@");
	assert(tok.type == RAW_ERROR);
	assert(tok.length == 1);
}

static void test_error_unexpected_backtick(void)
{
	RawToken tok = first_token("`");
	assert(tok.type == RAW_ERROR);
}

static void test_error_unterminated_string(void)
{
	RawToken tok = first_token("\"abc");
	assert(tok.type == RAW_ERROR);
}

static void test_error_unterminated_string_newline(void)
{
	RawToken tok = first_token("\"abc\n\"");
	assert(tok.type == RAW_ERROR);
}

static void test_error_unterminated_char(void)
{
	RawToken tok = first_token("'a");
	assert(tok.type == RAW_ERROR);
}

static void test_error_unterminated_block_comment(void)
{
	RawToken tok = first_token("/* abc");
	assert(tok.type == RAW_ERROR);
}

static void test_grammar_patterns(void)
{
	test_word_pattern();
	test_word_underscore_start();
	test_number_decimal();
	test_number_hex();
	test_number_octal();
	test_number_float_dot();
	test_number_float_exp();
	test_number_suffix_ul();
	test_string_simple();
	test_char_simple();
	test_hash();
	test_hash_hash();
	test_ellipsis();
	test_multi_char_symbols();
	test_single_char_symbols();
	test_eof();
	test_error_unexpected_char();
	test_error_unexpected_backtick();
	test_error_unterminated_string();
	test_error_unterminated_string_newline();
	test_error_unterminated_char();
	test_error_unterminated_block_comment();
	print_pass("1a.17 grammar pattern verification");
}

/* ─── 1a.18: C fragment tests ────────────────────────────────────── */

static void test_fragment_declaration(void)
{
	/* "int x = 42;" → WORD WORD ASSIGN NUMBER SEMICOLON EOF */
	RawTokenArray arr;
	tokenize_str("int x = 42;", &arr);

	size_t n = fun_array_RawToken_size(&arr);
	assert(n == 6);
	assert(fun_array_RawToken_get(&arr, 0).type == RAW_WORD);
	assert(fun_array_RawToken_get(&arr, 0).length == 3); /* "int" */
	assert(fun_array_RawToken_get(&arr, 1).type == RAW_WORD);
	assert(fun_array_RawToken_get(&arr, 1).length == 1); /* "x" */
	assert(fun_array_RawToken_get(&arr, 2).type == RAW_ASSIGN);
	assert(fun_array_RawToken_get(&arr, 3).type == RAW_NUMBER);
	assert(fun_array_RawToken_get(&arr, 3).length == 2); /* "42" */
	assert(fun_array_RawToken_get(&arr, 4).type == RAW_SEMICOLON);
	assert(fun_array_RawToken_get(&arr, 5).type == RAW_EOF);

	fun_array_RawToken_destroy(&arr);
}

static void test_fragment_arrow(void)
{
	/* "x->y" → WORD ARROW WORD EOF */
	RawTokenArray arr;
	tokenize_str("x->y", &arr);

	assert(fun_array_RawToken_size(&arr) == 4);
	assert(fun_array_RawToken_get(&arr, 0).type == RAW_WORD);
	assert(fun_array_RawToken_get(&arr, 1).type == RAW_ARROW);
	assert(fun_array_RawToken_get(&arr, 2).type == RAW_WORD);
	assert(fun_array_RawToken_get(&arr, 3).type == RAW_EOF);

	fun_array_RawToken_destroy(&arr);
}

static void test_fragment_post_increment(void)
{
	/* "x++" → WORD PLUS_PLUS EOF */
	RawTokenArray arr;
	tokenize_str("x++", &arr);

	assert(fun_array_RawToken_size(&arr) == 3);
	assert(fun_array_RawToken_get(&arr, 1).type == RAW_PLUS_PLUS);

	fun_array_RawToken_destroy(&arr);
}

static void test_fragment_lshift_assign(void)
{
	/* "<<=" → LSHIFT_ASSIGN EOF */
	RawTokenArray arr;
	tokenize_str("<<=", &arr);

	assert(fun_array_RawToken_size(&arr) == 2);
	assert(fun_array_RawToken_get(&arr, 0).type == RAW_LSHIFT_ASSIGN);
	assert(fun_array_RawToken_get(&arr, 0).length == 3);

	fun_array_RawToken_destroy(&arr);
}

static void test_fragment_ellipsis(void)
{
	/* "..." → ELLIPSIS EOF */
	RawTokenArray arr;
	tokenize_str("...", &arr);

	assert(fun_array_RawToken_size(&arr) == 2);
	assert(fun_array_RawToken_get(&arr, 0).type == RAW_ELLIPSIS);
	assert(fun_array_RawToken_get(&arr, 0).length == 3);

	fun_array_RawToken_destroy(&arr);
}

static void test_fragment_string_with_escape(void)
{
	/* "\"hello\\n\"" → RAW_STRING (length 9: " h e l l o \ n ") EOF */
	RawTokenArray arr;
	tokenize_str("\"hello\\n\"", &arr);

	assert(fun_array_RawToken_size(&arr) == 2);
	assert(fun_array_RawToken_get(&arr, 0).type == RAW_STRING);
	assert(fun_array_RawToken_get(&arr, 0).length == 9);

	fun_array_RawToken_destroy(&arr);
}

static void test_fragment_block_comment(void)
{
	/* "a /* comment * / b" → WORD WORD EOF */
	RawTokenArray arr;
	tokenize_str("a /* comment */ b", &arr);

	assert(fun_array_RawToken_size(&arr) == 3);
	assert(fun_array_RawToken_get(&arr, 0).type == RAW_WORD);
	assert(fun_array_RawToken_get(&arr, 1).type == RAW_WORD);
	assert(fun_array_RawToken_get(&arr, 2).type == RAW_EOF);

	fun_array_RawToken_destroy(&arr);
}

static void test_fragment_line_comment(void)
{
	/* "a // comment\nb" → WORD WORD EOF */
	RawTokenArray arr;
	tokenize_str("a // comment\nb", &arr);

	assert(fun_array_RawToken_size(&arr) == 3);
	assert(fun_array_RawToken_get(&arr, 0).type == RAW_WORD);
	assert(fun_array_RawToken_get(&arr, 1).type == RAW_WORD);
	assert(fun_array_RawToken_get(&arr, 2).type == RAW_EOF);

	fun_array_RawToken_destroy(&arr);
}

static void test_fragment_line_col_tracking(void)
{
	/* "int x = 42;" — token "x" is at col 5 */
	RawTokenArray arr;
	tokenize_str("int x = 42;", &arr);

	RawToken x_tok = fun_array_RawToken_get(&arr, 1);
	assert(x_tok.line == 1);
	assert(x_tok.col == 5);

	fun_array_RawToken_destroy(&arr);
}

static void test_fragment_multiline_col(void)
{
	/* "a\nb" → a at (1,1), b at (2,1) */
	RawTokenArray arr;
	tokenize_str("a\nb", &arr);

	RawToken a = fun_array_RawToken_get(&arr, 0);
	RawToken b = fun_array_RawToken_get(&arr, 1);
	assert(a.line == 1 && a.col == 1);
	assert(b.line == 2 && b.col == 1);

	fun_array_RawToken_destroy(&arr);
}

static void test_fragment_lexically_correct_error(void)
{
	// Lexically correct but semantically invalid code
	// must still tokenize without error.
	/* a+++++b  -> a++ ++ +b */
	RawTokenArray arr;
	tokenize_str("a+++++b", &arr);

	RawToken a = fun_array_RawToken_get(&arr, 0);
	RawToken a_plus_plus = fun_array_RawToken_get(&arr, 1);
	RawToken plus_plus = fun_array_RawToken_get(&arr, 2);
	RawToken plus = fun_array_RawToken_get(&arr, 3);
	RawToken b = fun_array_RawToken_get(&arr, 4);

	assert(a.type == RAW_WORD);
	assert(a.length == 1);
	assert(a_plus_plus.type == RAW_PLUS_PLUS);
	assert(a_plus_plus.length == 2);
	assert(plus_plus.type == RAW_PLUS_PLUS);
	assert(plus_plus.length == 2);
	assert(plus.type == RAW_PLUS);
	assert(plus.length == 1);
	assert(b.type == RAW_WORD);
	assert(b.length == 1);

	fun_array_RawToken_destroy(&arr);
}

static void test_c_fragments(void)
{
	test_fragment_declaration();
	test_fragment_arrow();
	test_fragment_post_increment();
	test_fragment_lshift_assign();
	test_fragment_ellipsis();
	test_fragment_string_with_escape();
	test_fragment_block_comment();
	test_fragment_line_comment();
	test_fragment_line_col_tracking();
	test_fragment_multiline_col();
	test_fragment_lexically_correct_error();
	print_pass("1a.18 C fragment tokenization");
}

/* ─── 1a.15 + 1a.16: Serialization round-trip ───────────────────── */

static void test_serialize_deserialize(void)
{
	RawTokenArray arr;
	tokenize_str("int x = 42;", &arr);
	size_t original_count = fun_array_RawToken_size(&arr);

	const char *tmp_path = "test_tmp.tokens";

	/* Serialize via tokenizer API */
	voidResult ser = tokenizer_serialize(&arr, tmp_path);
	assert(ser.error.code == 0);

	/* Validate binary format directly with fopen */
	FILE *f = fopen(tmp_path, "rb");
	assert(f != NULL);
	uint32_t magic = 0, count_in_file = 0;
	assert(fread(&magic, 4, 1, f) == 1);
	assert(fread(&count_in_file, 4, 1, f) == 1);
	fclose(f);
	assert(magic == 0x544F4B53u);
	assert(count_in_file == (uint32_t)original_count);

	/* Deserialize via tokenizer API */
	RawTokenArrayResult des = tokenizer_deserialize(tmp_path);
	assert(des.error.code == 0);
	assert(fun_array_RawToken_size(&des.value) == original_count);

	/* Compare every field */
	for (size_t i = 0; i < original_count; i++) {
		RawToken o = fun_array_RawToken_get(&arr, i);
		RawToken r = fun_array_RawToken_get(&des.value, i);
		assert(o.type == r.type);
		assert(o.offset == r.offset);
		assert(o.length == r.length);
		assert(o.line == r.line);
		assert(o.col == r.col);
	}

	fun_array_RawToken_destroy(&arr);
	fun_array_RawToken_destroy(&des.value);
	remove(tmp_path);

	print_pass("1a.15+1a.16 .tokens serialize/deserialize round-trip");
}

/* ─── 1a.19: Tokenize all source files ──────────────────────── */

#define MAX_DIR_LISTING 16384
static void test_real_source_files(void)
{
	const char *src_path = "../../src/commands/cmd_version.c";
	FILE *f = fopen(src_path, "rb");
	if (f == NULL) {
		printf("  (skipping 1a.19: cannot open %s)\n", src_path);
		return;
	}
	fseek(f, 0, SEEK_END);
	long file_size = ftell(f);
	fseek(f, 0, SEEK_SET);
	assert(file_size > 0 && file_size < 1024 * 1024);

	char *src_buf = (char *)malloc((size_t)file_size);
	assert(src_buf != NULL);
	size_t read_n = fread(src_buf, 1, (size_t)file_size, f);
	fclose(f);
	assert(read_n == (size_t)file_size);

	Tokenizer t;
	tokenizer_init(&t, src_buf, (uint32_t)file_size);

	RawTokenArrayResult res = fun_array_RawToken_create(512);
	assert(res.error.code == 0);

	int error_count = tokenizer_tokenize(&t, &res.value);
	size_t tok_count = fun_array_RawToken_size(&res.value);

	/* cmd_version.c starts with #include — first non-whitespace is '#' */
	assert(error_count == 0);
	assert(tok_count > 50);
	assert(fun_array_RawToken_get(&res.value, 0).type == RAW_HASH);

	fun_array_RawToken_destroy(&res.value);
	free(src_buf);

	print_pass("1a.19 tokenize actual source file (cmd_version.c)");
}

/* ─── main ───────────────────────────────────────────────────────── */

int main(void)
{
	printf("Running tokenizer tests:\n");
	test_grammar_patterns();
	test_c_fragments();
	test_serialize_deserialize();
	test_real_source_file();
	printf("All tests passed!\n");
	return 0;
}
