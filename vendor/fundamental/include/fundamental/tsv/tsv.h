#ifndef LIBRARY_TSV_H
#define LIBRARY_TSV_H

#include "../error/error.h"
#include <stddef.h>
#include <stdbool.h>

#define FUN_TSV_MAX_FIELDS 32

// Row of fields. Pointers are valid for the lifetime of the input buffer
// (it is modified in-place). Copy fields you need to keep.
typedef struct {
	const char **fields; // points into state->_fields
	size_t count;
} FunTsvRow;

// Parser state -- stack-allocatable (~280 bytes), no separate work memory.
typedef struct {
	char *_data;
	size_t _pos;
	size_t _len;
	const char *_fields[FUN_TSV_MAX_FIELDS];
} FunTsvState;

// Initialise TSV parser over a mutable, null-terminated buffer.
// The buffer will be modified in-place (\t and \n replaced with \0).
ErrorResult fun_tsv_init(FunTsvState *state, char *data);

// Advance to the next row. Populates row->fields and row->count.
// Returns value=true if a row was found, value=false at end of data.
boolResult fun_tsv_next(FunTsvState *state, FunTsvRow *row);

#endif // LIBRARY_TSV_H
