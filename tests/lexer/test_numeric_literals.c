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

/* ─── 1b.13: Numeric literal parsing tests ──────────────────────── */

static void test_decimal_integers(void)
{
	LexToken tok;

	/* Test: 0 */
	tok = first_lex_token("0");
	assert(tok.type == LEX_INT_LITERAL);
	assert(tok.value == 0);

	/* Test: 42 */
	tok = first_lex_token("42");
	assert(tok.type == LEX_INT_LITERAL);
	assert(tok.value == 42);

	/* Test: 123456789 */
	tok = first_lex_token("123456789");
	assert(tok.type == LEX_INT_LITERAL);
	assert(tok.value == 123456789);

	/* Test: 9999999999 (large decimal) */
	tok = first_lex_token("9999999999");
	assert(tok.type == LEX_INT_LITERAL);
	assert(tok.value == 9999999999ULL);

	print_pass("Decimal integers");
}

static void test_hex_integers(void)
{
	LexToken tok;

	/* Test: 0x0 */
	tok = first_lex_token("0x0");
	assert(tok.type == LEX_INT_LITERAL);
	assert(tok.value == 0);

	/* Test: 0xFF */
	tok = first_lex_token("0xFF");
	assert(tok.type == LEX_INT_LITERAL);
	assert(tok.value == 255);

	/* Test: 0xDEADBEEF */
	tok = first_lex_token("0xDEADBEEF");
	assert(tok.type == LEX_INT_LITERAL);
	assert(tok.value == 0xDEADBEEFULL);

	/* Test: 0x1234567890ABCDEF */
	tok = first_lex_token("0x1234567890ABCDEF");
	assert(tok.type == LEX_INT_LITERAL);
	assert(tok.value == 0x1234567890ABCDEFULL);

	print_pass("Hex integers");
}

static void test_octal_integers(void)
{
	LexToken tok;

	/* Test: 00 */
	tok = first_lex_token("00");
	assert(tok.type == LEX_INT_LITERAL);
	assert(tok.value == 0);

	/* Test: 0777 */
	tok = first_lex_token("0777");
	assert(tok.type == LEX_INT_LITERAL);
	assert(tok.value == 511); /* 7*64 + 7*8 + 7 */

	/* Test: 01234567 */
	tok = first_lex_token("01234567");
	assert(tok.type == LEX_INT_LITERAL);
	assert(tok.value == 01234567);

	print_pass("Octal integers");
}

static void test_float_literals(void)
{
	LexToken tok;

	/* Test: 3.14 */
	tok = first_lex_token("3.14");
	assert(tok.type == LEX_FLOAT_LITERAL);

	/* Test: 0.0 */
	tok = first_lex_token("0.0");
	assert(tok.type == LEX_FLOAT_LITERAL);

	/* Test: 1e10 */
	tok = first_lex_token("1e10");
	assert(tok.type == LEX_FLOAT_LITERAL);

	/* Test: 2.5e-3 */
	tok = first_lex_token("2.5e-3");
	assert(tok.type == LEX_FLOAT_LITERAL);

	/* Test: 1.0f */
	tok = first_lex_token("1.0f");
	assert(tok.type == LEX_FLOAT_LITERAL);

	print_pass("Float literals");
}

static void test_numeric_suffixes(void)
{
	LexToken tok;

	/* Test: 42u */
	tok = first_lex_token("42u");
	assert(tok.type == LEX_INT_LITERAL);
	assert(tok.value == 42);

	/* Test: 42U */
	tok = first_lex_token("42U");
	assert(tok.type == LEX_INT_LITERAL);
	assert(tok.value == 42);

	/* Test: 42UL */
	tok = first_lex_token("42UL");
	assert(tok.type == LEX_INT_LITERAL);
	assert(tok.value == 42);

	/* Test: 3.14f */
	tok = first_lex_token("3.14f");
	assert(tok.type == LEX_FLOAT_LITERAL);

	print_pass("Numeric suffixes (u, U, L, UL, f)");
}

/* ─── Main ───────────────────────────────────────────────────────── */

int main(void)
{
	printf("Running lexer numeric literal tests:\n");
	test_decimal_integers();
	test_hex_integers();
	test_octal_integers();
	test_float_literals();
	test_numeric_suffixes();
	printf("All numeric literal tests passed!\n");
	return 0;
}
