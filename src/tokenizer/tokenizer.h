#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <stdint.h>

#include "fundamental/array/array.h"

/* ─── Token types (Pass 1: language-agnostic) ────────────────────── */
/* Matches patterns defined in src/tokenizer/tokenizer.grammar       */

typedef enum {
	/* Words and identifiers (C keyword classification is Pass 2) */
	RAW_WORD,
	/* Numeric literals (value parsing is Pass 2) */
	RAW_NUMBER,
	/* String literals — raw bytes, escape sequences not resolved */
	RAW_STRING,
	/* Character literals — raw bytes, escape sequences not resolved */
	RAW_CHAR,
	/* Preprocessor directive markers */
	RAW_HASH, /* #  */
	RAW_HASH_HASH, /* ## */
	/* Ellipsis */
	RAW_ELLIPSIS, /* ... */
	/* Multi-character compound-assignment operators */
	RAW_LSHIFT_ASSIGN, /* <<= */
	RAW_RSHIFT_ASSIGN, /* >>= */
	RAW_PLUS_ASSIGN, /* +=  */
	RAW_MINUS_ASSIGN, /* -=  */
	RAW_STAR_ASSIGN, /* *=  */
	RAW_SLASH_ASSIGN, /* /=  */
	RAW_PERCENT_ASSIGN, /* %=  */
	RAW_AMP_ASSIGN, /* &=  */
	RAW_PIPE_ASSIGN, /* |=  */
	RAW_CARET_ASSIGN, /* ^=  */
	/* Comparison operators */
	RAW_EQ_EQ, /* == */
	RAW_BANG_EQ, /* != */
	RAW_LT_EQ, /* <= */
	RAW_GT_EQ, /* >= */
	/* Logical operators */
	RAW_AMP_AMP, /* && */
	RAW_PIPE_PIPE, /* || */
	/* Shift operators */
	RAW_LSHIFT, /* << */
	RAW_RSHIFT, /* >> */
	/* Increment / decrement */
	RAW_PLUS_PLUS, /* ++ */
	RAW_MINUS_MINUS, /* -- */
	/* Arrow */
	RAW_ARROW, /* -> */
	/* Single-character operators and punctuation */
	RAW_PLUS, /* + */
	RAW_MINUS, /* - */
	RAW_STAR, /* * */
	RAW_SLASH, /* / */
	RAW_PERCENT, /* % */
	RAW_AMPERSAND, /* & */
	RAW_PIPE, /* | */
	RAW_CARET, /* ^ */
	RAW_TILDE, /* ~ */
	RAW_BANG, /* ! */
	RAW_ASSIGN, /* = */
	RAW_LT, /* < */
	RAW_GT, /* > */
	RAW_DOT, /* . */
	RAW_QUESTION, /* ? */
	RAW_COLON, /* : */
	RAW_SEMICOLON, /* ; */
	RAW_COMMA, /* , */
	RAW_LPAREN, /* ( */
	RAW_RPAREN, /* ) */
	RAW_LBRACE, /* { */
	RAW_RBRACE, /* } */
	RAW_LBRACKET, /* [ */
	RAW_RBRACKET, /* ] */
	/* Sentinels */
	RAW_EOF, /* end of input */
	RAW_ERROR, /* unexpected character or unterminated literal/comment */
} RawTokenType;

/* ─── Token record ───────────────────────────────────────────────── */

typedef struct {
	RawTokenType type;
	uint32_t offset; /* byte offset into source buffer */
	uint32_t length; /* byte length of token text */
	uint32_t line; /* 1-based line number of token start */
	uint32_t col; /* 1-based column number of token start */
} RawToken;

DEFINE_ARRAY_TYPE(RawToken)

/* ─── Tokenizer cursor ───────────────────────────────────────────── */

typedef struct {
	const char *source; /* source buffer (not owned) */
	uint32_t source_len; /* byte length of source buffer */
	uint32_t offset; /* current cursor position in source */
	uint32_t line; /* 1-based line at cursor */
	uint32_t col; /* 1-based column at cursor */
} Tokenizer;

/* ─── Public API ─────────────────────────────────────────────────── */

/*
 * Initialise a tokenizer over [source, source+length).
 * The buffer must remain valid for the tokenizer's lifetime.
 * Cursor starts at offset 0, line 1, column 1.
 */
void tokenizer_init(Tokenizer *t, const char *source, uint32_t length);

/*
 * Consume and return the next token.
 * Leading whitespace and comments are silently skipped.
 * Returns RAW_ERROR for unexpected characters or unterminated
 * string/char literals or block comments.
 * Returns RAW_EOF when the buffer is exhausted.
 */
RawToken tokenizer_next(Tokenizer *t);

/*
 * Tokenize the entire buffer into *out (caller-created, caller-destroyed).
 * Appends tokens including a terminal RAW_EOF token.
 * Returns the number of RAW_ERROR tokens produced (0 = clean input).
 * Returns -1 if array growth fails (out of memory).
 */
int tokenizer_tokenize(Tokenizer *t, RawTokenArray *out);

/* ─── Serialization (.tokens binary format) ──────────────────────── */
/*
 * Binary layout:
 *   [4 bytes]  magic   = 0x544F4B53 ("TOKS")
 *   [4 bytes]  count   = number of RawToken records (uint32_t)
 *   [count*20] records = packed RawToken structs (5 × uint32_t each)
 */

/*
 * Write a RawTokenArray to a .tokens binary file.
 */
voidResult tokenizer_serialize(const RawTokenArray *tokens, const char *path);

/*
 * Read a .tokens binary file back into a new RawTokenArray.
 * Caller must call fun_array_RawToken_destroy() on the returned value.
 */
RawTokenArrayResult tokenizer_deserialize(const char *path);

#endif /* TOKENIZER_H */
