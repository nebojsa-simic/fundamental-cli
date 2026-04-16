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

/* ─── Helper: verify keyword classification ─────────────────────── */

static int verify_keyword(const char *text, LexTokenType expected)
{
	Tokenizer t;
	tokenizer_init(&t, text, fun_string_length(text));
	RawToken raw = tokenizer_next(&t);

	Lexer l;
	lexer_init(&l, &raw, 1, text, fun_string_length(text));
	LexToken lex = lexer_next(&l);

	return lex.type == expected;
}

/* ─── 1b.12: Keyword classification tests ───────────────────────── */

static void test_type_keywords(void)
{
	assert(verify_keyword("int", LEX_INT));
	assert(verify_keyword("char", LEX_CHAR));
	assert(verify_keyword("void", LEX_VOID));
	assert(verify_keyword("float", LEX_FLOAT));
	assert(verify_keyword("double", LEX_DOUBLE));
	assert(verify_keyword("struct", LEX_STRUCT));
	assert(verify_keyword("union", LEX_UNION));
	assert(verify_keyword("enum", LEX_ENUM));
	assert(verify_keyword("typedef", LEX_TYPEDEF));
	print_pass("Type keywords");
}

static void test_control_keywords(void)
{
	assert(verify_keyword("if", LEX_IF));
	assert(verify_keyword("else", LEX_ELSE));
	assert(verify_keyword("for", LEX_FOR));
	assert(verify_keyword("while", LEX_WHILE));
	assert(verify_keyword("do", LEX_DO));
	assert(verify_keyword("switch", LEX_SWITCH));
	assert(verify_keyword("case", LEX_CASE));
	assert(verify_keyword("default", LEX_DEFAULT));
	assert(verify_keyword("break", LEX_BREAK));
	assert(verify_keyword("continue", LEX_CONTINUE));
	assert(verify_keyword("return", LEX_RETURN));
	print_pass("Control flow keywords");
}

static void test_storage_keywords(void)
{
	assert(verify_keyword("static", LEX_STATIC));
	assert(verify_keyword("const", LEX_CONST));
	assert(verify_keyword("inline", LEX_INLINE));
	assert(verify_keyword("extern", LEX_EXTERN));
	assert(verify_keyword("volatile", LEX_VOLATILE));
	assert(verify_keyword("restrict", LEX_RESTRICT));
	assert(verify_keyword("goto", LEX_GOTO));
	print_pass("Storage class keywords");
}

static void test_identifier_keywords(void)
{
	assert(verify_keyword("sizeof", LEX_SIZEOF));
	assert(verify_keyword("signed", LEX_SIGNED));
	assert(verify_keyword("unsigned", LEX_UNSIGNED));
	assert(verify_keyword("short", LEX_SHORT));
	assert(verify_keyword("long", LEX_LONG));
	print_pass("Operator and modifier keywords");
}

static void test_identifiers(void)
{
	assert(verify_keyword("foo", LEX_IDENTIFIER));
	assert(verify_keyword("myVar", LEX_IDENTIFIER));
	assert(verify_keyword("_test", LEX_IDENTIFIER));
	assert(verify_keyword("__LINE__", LEX_IDENTIFIER));
	print_pass("Identifiers (non-keywords)");
}

/* ─── Main ───────────────────────────────────────────────────────── */

int main(void)
{
	printf("Running lexer keyword tests:\n");
	test_type_keywords();
	test_control_keywords();
	test_storage_keywords();
	test_identifier_keywords();
	test_identifiers();
	printf("All keyword tests passed!\n");
	return 0;
}
