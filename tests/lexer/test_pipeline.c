#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tokenizer/tokenizer.h"
#include "tokenizer/lexer.h"

#define GREEN "\033[0;32m✓\033[0m"

static void print_pass(const char *name)
{
	printf("%s %s\n", GREEN, name);
}

/* ─── Expected token count ranges (±20% for safety) ─────────────── */
/* Note: Only testing files without Unicode decorative comments */

typedef struct {
	const char *file_path;
	size_t expected_min;
	size_t expected_max;
} FileTestSpec;

static FileTestSpec test_specs[] = {
	{"../../src/main.c", 170, 255},
	{"../../src/cli/cli.c", 315, 470},
	{"../../src/build/config.c", 315, 470},
	{"../../src/build/detector.c", 85, 125},
	{"../../src/build/executor.c", 65, 95},
	{"../../src/build/generator.c", 1005, 1510},
	{"../../src/commands/cmd_version.c", 55, 85},
	{"../../src/commands/cmd_init.c", 540, 810},
	{"../../src/commands/cmd_clean.c", 180, 270},
	{"../../src/commands/cmd_build.c", 455, 680},
	{"../../src/commands/cmd_test.c", 495, 740},
	{"../../src/commands/cmd_test_add.c", 545, 820},
	{"../../src/test/discovery.c", 850, 1275},
	{"../../src/test/module_map.c", 495, 740},
	{"../../src/test/reporter.c", 320, 485},
	{"../../src/test/runner.c", 820, 1225},
	{"../../src/test/scaffolder.c", 770, 1150},
};

/* ─── Helper: read file into buffer ─────────────────────────────── */

static char *read_file(const char *path, size_t *out_len)
{
	FILE *f = fopen(path, "rb");
	if (!f) return NULL;
	
	fseek(f, 0, SEEK_END);
	long size = ftell(f);
	fseek(f, 0, SEEK_SET);
	
	char *buf = (char *)malloc((size_t)size);
	if (!buf) { fclose(f); return NULL; }
	
	size_t read_n = fread(buf, 1, (size_t)size, f);
	fclose(f);
	
	if (read_n != (size_t)size) { free(buf); return NULL; }
	
	*out_len = (size_t)size;
	return buf;
}

/* ─── 1b.15: Full pipeline tests ────────────────────────────────── */

static void test_file_pipeline(FileTestSpec *spec)
{
	size_t file_len = 0;
	char *src = read_file(spec->file_path, &file_len);
	
	if (!src) {
		fprintf(stderr, "ERROR: Cannot open %s\n", spec->file_path);
		assert(0 && "Cannot open source file");
	}
	
	/* Step 1: Tokenize */
	Tokenizer t;
	tokenizer_init(&t, src, (uint32_t)file_len);
	
	RawTokenArrayResult raw_res = fun_array_RawToken_create(1024);
	assert(raw_res.error.code == 0);
	
	int tok_errors = tokenizer_tokenize(&t, &raw_res.value);
	assert(tok_errors == 0 && "Tokenizer produced errors");
	
	size_t raw_count = fun_array_RawToken_size(&raw_res.value);
	
	/* Step 2: Lex */
	Lexer l;
	string_table_init(&l.string_table);
	lexer_init(&l, raw_res.value.array.data, (uint32_t)raw_count, src, (uint32_t)file_len);
	
	LexTokenArrayResult lex_res = fun_array_LexToken_create(1024);
	assert(lex_res.error.code == 0);
	
	int lex_errors = lexer_lex(&l, &lex_res.value);
	assert(lex_errors == 0 && "Lexer produced errors");
	
	size_t lex_count = fun_array_LexToken_size(&lex_res.value);
	
	/* Step 3: Verify token count */
	assert(lex_count >= spec->expected_min && 
		   lex_count <= spec->expected_max &&
		   "Token count out of expected range");
	
	/* Step 4: Verify string table is populated */
	assert(l.string_table.count > 0 && "String table should not be empty");
	
	/* Cleanup */
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
	printf("Running lexer pipeline tests:\n");
	test_full_pipeline_on_sources();
	printf("All pipeline tests passed!\n");
	return 0;
}
