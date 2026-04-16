#ifndef LEXER_H
#define LEXER_H

#include <stdint.h>

#include "fundamental/array/array.h"
#include "tokenizer/tokenizer.h"

/* ─── Lex token types (Pass 2: C-specific classification) ────────── */
/* Keywords */
typedef enum {
	/* Type keywords */
	LEX_INT,
	LEX_CHAR,
	LEX_VOID,
	LEX_FLOAT,
	LEX_DOUBLE,
	LEX_STRUCT,
	LEX_UNION,
	LEX_ENUM,
	LEX_TYPEDEF,
	LEX_SIGNED,
	LEX_UNSIGNED,
	LEX_SHORT,
	LEX_LONG,

	/* Control flow keywords */
	LEX_IF,
	LEX_ELSE,
	LEX_FOR,
	LEX_WHILE,
	LEX_DO,
	LEX_SWITCH,
	LEX_CASE,
	LEX_DEFAULT,
	LEX_BREAK,
	LEX_CONTINUE,
	LEX_RETURN,
	LEX_GOTO,

	/* Storage class and type qualifiers */
	LEX_STATIC,
	LEX_CONST,
	LEX_INLINE,
	LEX_EXTERN,
	LEX_VOLATILE,
	LEX_RESTRICT,

	/* Operators and built-ins */
	LEX_SIZEOF,
	LEX_ALIGNOF,
	LEX__GENERIC,

	/* Identifier (RAW_WORD that is not a keyword) */
	LEX_IDENTIFIER,

	/* Literals */
	LEX_INT_LITERAL,
	LEX_FLOAT_LITERAL,
	LEX_CHAR_LITERAL,
	LEX_STRING_LITERAL,

	/* Operators — carried forward from tokenizer with semantic names */
	/* Arithmetic */
	LEX_PLUS,
	LEX_MINUS,
	LEX_STAR,
	LEX_SLASH,
	LEX_PERCENT,

	/* Bitwise */
	LEX_AMPERSAND,
	LEX_PIPE,
	LEX_CARET,
	LEX_TILDE,

	/* Logical */
	LEX_BANG,
	LEX_AMP_AMP,
	LEX_PIPE_PIPE,

	/* Comparison */
	LEX_ASSIGN,
	LEX_EQ,
	LEX_NEQ,
	LEX_LT,
	LEX_GT,
	LEX_LTE,
	LEX_GTE,

	/* Assignment operators */
	LEX_PLUS_ASSIGN,
	LEX_MINUS_ASSIGN,
	LEX_STAR_ASSIGN,
	LEX_SLASH_ASSIGN,
	LEX_PERCENT_ASSIGN,
	LEX_AMP_ASSIGN,
	LEX_PIPE_ASSIGN,
	LEX_CARET_ASSIGN,
	LEX_LSHIFT_ASSIGN,
	LEX_RSHIFT_ASSIGN,

	/* Shift operators */
	LEX_LSHIFT,
	LEX_RSHIFT,

	/* Increment / decrement */
	LEX_INCREMENT,
	LEX_DECREMENT,

	/* Member access */
	LEX_ARROW,
	LEX_DOT,

	/* Ternary */
	LEX_QUESTION,
	LEX_COLON,

	/* Punctuation */
	LEX_LPAREN,
	LEX_RPAREN,
	LEX_LBRACE,
	LEX_RBRACE,
	LEX_LBRACKET,
	LEX_RBRACKET,
	LEX_SEMICOLON,
	LEX_COMMA,
	LEX_ELLIPSIS,

	/* Preprocessor */
	LEX_HASH,
	LEX_HASH_HASH,

	/* Special */
	LEX_EOF,
	LEX_ERROR,
} LexTokenType;

/* ─── Lex token ──────────────────────────────────────────────────── */

typedef struct {
	LexTokenType type;
	uint32_t offset; /* byte offset in original source */
	uint32_t length; /* byte length in original source */
	uint32_t line; /* 1-based line number */
	uint32_t col; /* 1-based column number */
	uint64_t value; /* int value, double bits, or string table offset */
} LexToken;

DEFINE_ARRAY_TYPE(LexToken)

/* ─── String table ───────────────────────────────────────────────── */

typedef struct {
	char *data; /* contiguous block of null-terminated strings */
	uint32_t size; /* total bytes allocated */
	uint32_t count; /* number of strings */
	uint32_t capacity; /* allocated capacity */
} StringTable;

/*
 * Initialise an empty string table.
 */
void string_table_init(StringTable *st);

/*
 * Add a string to the table, return its offset.
 */
uint32_t string_table_add(StringTable *st, const char *str, uint32_t len);

/*
 * Get a string by offset.
 */
const char *string_table_get(const StringTable *st, uint32_t offset);

/*
 * Free the string table.
 */
void string_table_destroy(StringTable *st);

/* ─── Lexer cursor ───────────────────────────────────────────────── */

typedef struct {
	const RawToken *raw_tokens; /* not owned */
	uint32_t raw_count;
	uint32_t index; /* current token index */
	const char *source; /* original source buffer (not owned) */
	uint32_t source_len;
	StringTable string_table;
} Lexer;

/* ─── Public API ─────────────────────────────────────────────────── */

/*
 * Initialise a lexer over an array of RawTokens.
 * The source buffer is needed to extract WORD text for keyword classification.
 */
void lexer_init(Lexer *l, const RawToken *raw_tokens, uint32_t raw_count,
				const char *source, uint32_t source_len);

/*
 * Classify the next raw token and return the classified LexToken.
 * Returns LEX_EOF when exhausted, LEX_ERROR for invalid literals.
 */
LexToken lexer_next(Lexer *l);

/*
 * Lex the entire token stream into *out (caller-created, caller-destroyed).
 * Appends tokens including a terminal LEX_EOF token.
 * Returns the number of LEX_ERROR tokens produced (0 = clean input).
 * Returns -1 if array growth fails (out of memory).
 */
int lexer_lex(Lexer *l, LexTokenArray *out);

/* ─── Serialization (.lex binary format) ─────────────────────────── */
/*
 * Binary layout:
 *   [4 bytes]  magic   = 0x4C455846 ("LEXF")
 *   [4 bytes]  count   = number of LexToken records (uint32_t)
 *   [4 bytes]  st_size = string table size in bytes (uint32_t)
 *   [count*28] tokens  = packed LexToken structs (7 × uint32_t each)
 *   [st_size]  strings = null-separated string table
 */

/*
 * Write a LexTokenArray and StringTable to a .lex binary file.
 */
voidResult lexer_serialize(const LexTokenArray *tokens, const StringTable *st,
						   const char *path);

/*
 * Read a .lex binary file back into a new LexTokenArray and StringTable.
 * Caller must call fun_array_LexToken_destroy() and string_table_destroy().
 */
typedef struct {
	LexTokenArray tokens;
	StringTable string_table;
	ErrorResult error;
} LexResult;

LexResult lexer_deserialize(const char *path);

#endif /* LEXER_H */
