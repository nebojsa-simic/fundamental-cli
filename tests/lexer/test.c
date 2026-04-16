/* Lexer test runner - unified test suite */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tokenizer/tokenizer.h"
#include "tokenizer/lexer.h"
#include "fundamental/string/string.h"

#define GREEN "\033[0;32m✓\033[0m"

static void print_pass(const char *name)
{
	printf("%s %s\n", GREEN, name);
}

/* ─── Keyword tests (from test_keywords.c) ─────────────────────── */

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

/* ─── Numeric literal tests (from test_numeric_literals.c) ──────── */

static LexToken first_lex_token(String src)
{
	Tokenizer t;
	tokenizer_init(&t, src, fun_string_length(src));
	RawToken raw = tokenizer_next(&t);

	Lexer l;
	lexer_init(&l, &raw, 1, src, fun_string_length(src));
	return lexer_next(&l);
}

static void test_integer_literals(void)
{
	LexToken tok = first_lex_token("42");
	assert(tok.type == LEX_INT_LITERAL);
	assert(tok.value == 42);

	tok = first_lex_token("0xFF");
	assert(tok.type == LEX_INT_LITERAL);
	assert(tok.value == 255);

	tok = first_lex_token("0777");
	assert(tok.type == LEX_INT_LITERAL);
	assert(tok.value == 511);

	tok = first_lex_token("100UL");
	assert(tok.type == LEX_INT_LITERAL);
	assert(tok.value == 100);

	print_pass("Integer literals");
}

static void test_floating_literals(void)
{
	LexToken tok = first_lex_token("3.14");
	assert(tok.type == LEX_FLOAT_LITERAL);

	tok = first_lex_token("1e10");
	assert(tok.type == LEX_FLOAT_LITERAL);

	tok = first_lex_token("2.5e-3");
	assert(tok.type == LEX_FLOAT_LITERAL);

	print_pass("Floating point literals");
}

static void test_numeric_suffixes(void)
{
	LexToken tok = first_lex_token("42u");
	assert(tok.type == LEX_INT_LITERAL);
	assert(tok.value == 42);

	tok = first_lex_token("42UL");
	assert(tok.type == LEX_INT_LITERAL);
	assert(tok.value == 42);

	tok = first_lex_token("3.14f");
	assert(tok.type == LEX_FLOAT_LITERAL);

	print_pass("Numeric suffixes");
}

/* ─── Escape sequence tests (from test_escape_sequences.c) ──────── */

static void test_string_escapes(void)
{
	LexToken tok = first_lex_token("\"hello\\nworld\"");
	assert(tok.type == LEX_STRING_LITERAL);

	tok = first_lex_token("\"tab\\there\"");
	assert(tok.type == LEX_STRING_LITERAL);

	tok = first_lex_token("\"quote\\\"here\"");
	assert(tok.type == LEX_STRING_LITERAL);

	print_pass("String escape sequences");
}

static void test_char_escapes(void)
{
	LexToken tok = first_lex_token("'\\n'");
	assert(tok.type == LEX_CHAR_LITERAL);

	tok = first_lex_token("'\\t'");
	assert(tok.type == LEX_CHAR_LITERAL);

	tok = first_lex_token("'\\\\'");
	assert(tok.type == LEX_CHAR_LITERAL);

	print_pass("Char escape sequences");
}

static void test_invalid_escapes(void)
{
	/* String escape errors are currently ignored by the lexer */
	LexToken tok = first_lex_token("\"\\x\"");
	assert(tok.type == LEX_STRING_LITERAL);

	/* Char literal escape errors DO return LEX_ERROR */
	tok = first_lex_token("'\\z'");
	assert(tok.type == LEX_ERROR);

	print_pass("Escape sequence handling");
}

/* ─── Pipeline tests (from test_pipeline.c) ─────────────────────── */

typedef struct {
	const char *file_path;
	size_t expected_min;
	size_t expected_max;
} FileTestSpec;

static FileTestSpec test_specs[] = {
	{ "../../src/main.c", 170, 255 },
	{ "../../src/cli/cli.c", 315, 470 },
	{ "../../src/build/config.c", 315, 470 },
	{ "../../src/build/detector.c", 85, 125 },
	{ "../../src/build/executor.c", 65, 95 },
	{ "../../src/build/generator.c", 1005, 1510 },
	{ "../../src/commands/cmd_version.c", 55, 85 },
	{ "../../src/commands/cmd_init.c", 540, 810 },
	{ "../../src/commands/cmd_clean.c", 180, 270 },
	{ "../../src/commands/cmd_build.c", 455, 680 },
	{ "../../src/commands/cmd_test.c", 495, 740 },
	{ "../../src/commands/cmd_test_add.c", 545, 820 },
	{ "../../src/test/discovery.c", 850, 1275 },
	{ "../../src/test/module_map.c", 495, 740 },
	{ "../../src/test/reporter.c", 320, 485 },
	{ "../../src/test/runner.c", 820, 1225 },
	{ "../../src/test/scaffolder.c", 770, 1150 },
};

static char *read_file(const char *path, size_t *out_len)
{
	FILE *f = fopen(path, "rb");
	if (!f)
		return NULL;

	fseek(f, 0, SEEK_END);
	long size = ftell(f);
	fseek(f, 0, SEEK_SET);

	char *buf = (char *)malloc((size_t)size);
	if (!buf) {
		fclose(f);
		return NULL;
	}

	size_t read_n = fread(buf, 1, (size_t)size, f);
	fclose(f);

	if (read_n != (size_t)size) {
		free(buf);
		return NULL;
	}

	*out_len = (size_t)size;
	return buf;
}

static void test_file_pipeline(FileTestSpec *spec)
{
	size_t file_len = 0;
	char *src = read_file(spec->file_path, &file_len);

	if (!src) {
		fprintf(stderr, "ERROR: Cannot open %s\n", spec->file_path);
		assert(0 && "Cannot open source file");
	}

	Tokenizer t;
	tokenizer_init(&t, src, (uint32_t)file_len);

	RawTokenArrayResult raw_res = fun_array_RawToken_create(1024);
	assert(raw_res.error.code == 0);

	int tok_errors = tokenizer_tokenize(&t, &raw_res.value);
	assert(tok_errors == 0 && "Tokenizer produced errors");

	size_t raw_count = fun_array_RawToken_size(&raw_res.value);

	Lexer l;
	string_table_init(&l.string_table);
	lexer_init(&l, raw_res.value.array.data, (uint32_t)raw_count, src,
			   (uint32_t)file_len);

	LexTokenArrayResult lex_res = fun_array_LexToken_create(1024);
	assert(lex_res.error.code == 0);

	int lex_errors = lexer_lex(&l, &lex_res.value);
	assert(lex_errors == 0 && "Lexer produced errors");

	size_t lex_count = fun_array_LexToken_size(&lex_res.value);

	assert(lex_count >= spec->expected_min && lex_count <= spec->expected_max &&
		   "Token count out of expected range");

	assert(l.string_table.count > 0 && "String table should not be empty");

	fun_array_RawToken_destroy(&raw_res.value);
	fun_array_LexToken_destroy(&lex_res.value);
	string_table_destroy(&l.string_table);
	free(src);

	printf("  ✓ Full pipeline: %s (%zu tokens)\n", spec->file_path, lex_count);
}

static void test_full_pipeline_on_sources(void)
{
	int num_specs = sizeof(test_specs) / sizeof(test_specs[0]);

	for (int i = 0; i < num_specs; i++) {
		test_file_pipeline(&test_specs[i]);
	}

	print_pass("Full pipeline on 15 source files");
}

/* ─── Main ───────────────────────────────────────────────────────── */

int main(void)
{
	printf("Running lexer tests:\n");

	printf("\nKeyword tests:\n");
	test_type_keywords();
	test_control_keywords();
	test_storage_keywords();
	test_identifier_keywords();
	test_identifiers();

	printf("\nNumeric literal tests:\n");
	test_integer_literals();
	test_floating_literals();
	test_numeric_suffixes();

	printf("\nEscape sequence tests:\n");
	test_string_escapes();
	test_char_escapes();
	test_invalid_escapes();

	printf("\nPipeline tests:\n");
	test_full_pipeline_on_sources();

	printf("\nAll lexer tests passed!\n");
	return 0;
}
