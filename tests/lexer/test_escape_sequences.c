#include <assert.h>
#include <stdio.h>

#include "tokenizer/tokenizer.h"
#include "tokenizer/lexer.h"
#include "fundamental/string/string.h"

#define GREEN "\033[0;32m✓\033[0m"

static void print_pass(const char *name)
{
	printf("%s %s\n", GREEN, name);
}

/* ─── Helper: get first lex token from string ───────────────────── */

static LexToken first_lex_token(String src)
{
	Tokenizer t;
	tokenizer_init(&t, src, fun_string_length(src));
	RawToken raw = tokenizer_next(&t);

	Lexer l;
	lexer_init(&l, &raw, 1, src, fun_string_length(src));
	return lexer_next(&l);
}

/* ─── 1b.14: Escape sequence resolution tests ───────────────────── */

static void test_character_literals(void)
{
	LexToken tok;

	/* Test: 'a' */
	tok = first_lex_token("'a'");
	assert(tok.type == LEX_CHAR_LITERAL);
	assert(tok.value == 'a');

	/* Test: '\n' */
	tok = first_lex_token("'\\n'");
	assert(tok.type == LEX_CHAR_LITERAL);
	assert(tok.value == '\n');

	/* Test: '\t' */
	tok = first_lex_token("'\\t'");
	assert(tok.type == LEX_CHAR_LITERAL);
	assert(tok.value == '\t');

	/* Test: '\0' */
	tok = first_lex_token("'\\0'");
	assert(tok.type == LEX_CHAR_LITERAL);
	assert(tok.value == '\0');

	/* Test: '\\' */
	tok = first_lex_token("'\\\\'");
	assert(tok.type == LEX_CHAR_LITERAL);
	assert(tok.value == '\\');

	/* Test: '\'' */
	tok = first_lex_token("'\\''");
	assert(tok.type == LEX_CHAR_LITERAL);
	assert(tok.value == '\'');

	/* Test: '\"' */
	tok = first_lex_token("'\\\"'");
	assert(tok.type == LEX_CHAR_LITERAL);
	assert(tok.value == '\"');

	print_pass("Character literals");
}

static void test_hex_escapes(void)
{
	LexToken tok;

	/* Test: '\x41' (A = 65) */
	tok = first_lex_token("'\\x41'");
	assert(tok.type == LEX_CHAR_LITERAL);
	assert(tok.value == 'A');

	/* Test: '\xFF' (255) */
	tok = first_lex_token("'\\xFF'");
	assert(tok.type == LEX_CHAR_LITERAL);
	assert(tok.value == 255);

	/* Test: '\x00' (0) */
	tok = first_lex_token("'\\x00'");
	assert(tok.type == LEX_CHAR_LITERAL);
	assert(tok.value == 0);

	print_pass("Hex escape sequences");
}

static void test_octal_escapes(void)
{
	LexToken tok;

	/* Test: '\0' (0) */
	tok = first_lex_token("'\\0'");
	assert(tok.type == LEX_CHAR_LITERAL);
	assert(tok.value == 0);

	/* Test: '\101' (A = 65) */
	tok = first_lex_token("'\\101'");
	assert(tok.type == LEX_CHAR_LITERAL);
	assert(tok.value == 'A');

	/* Test: '\377' (255) */
	tok = first_lex_token("'\\377'");
	assert(tok.type == LEX_CHAR_LITERAL);
	assert(tok.value == 255);

	print_pass("Octal escape sequences");
}

static void test_string_escapes(void)
{
	LexToken tok;

	/* Test: "\n" */
	tok = first_lex_token("\"\\n\"");
	assert(tok.type == LEX_STRING_LITERAL);

	/* Test: "\t" */
	tok = first_lex_token("\"\\t\"");
	assert(tok.type == LEX_STRING_LITERAL);

	/* Test: "\\" */
	tok = first_lex_token("\"\\\\\"");
	assert(tok.type == LEX_STRING_LITERAL);

	/* Test: "\"" */
	tok = first_lex_token("\"\\\"\"");
	assert(tok.type == LEX_STRING_LITERAL);

	/* Test: "\x41" (contains 'A') */
	tok = first_lex_token("\"\\x41\"");
	assert(tok.type == LEX_STRING_LITERAL);

	print_pass("String escape sequences");
}

/* ─── Main ───────────────────────────────────────────────────────── */

int main(void)
{
	printf("Running lexer escape sequence tests:\n");
	test_character_literals();
	test_hex_escapes();
	test_octal_escapes();
	test_string_escapes();
	printf("All escape sequence tests passed!\n");
	return 0;
}
