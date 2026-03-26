#include "fundamental/tsv/tsv.h"
#include "fundamental/string/string.h"

ErrorResult fun_tsv_init(FunTsvState *state, char *data)
{
	if (state == NULL || data == NULL) {
		return ERROR_RESULT_NULL_POINTER;
	}

	state->_data = data;
	state->_pos = 0;
	state->_len = (size_t)fun_string_length(data);

	return ERROR_RESULT_NO_ERROR;
}

boolResult fun_tsv_next(FunTsvState *state, FunTsvRow *row)
{
	boolResult result;
	result.error = ERROR_RESULT_NO_ERROR;
	result.value = false;

	if (state == NULL || row == NULL) {
		result.error = ERROR_RESULT_NULL_POINTER;
		return result;
	}

	// Loop to skip empty rows
	while (state->_pos < state->_len) {
		size_t row_start = state->_pos;

		// Find end of this row (\n or end of data)
		size_t scan = state->_pos;
		while (scan < state->_len && state->_data[scan] != '\n') {
			scan++;
		}

		// Replace \n with \0 and advance past it
		if (scan < state->_len) {
			state->_data[scan] = '\0';
		}
		state->_pos = scan + 1;

		// Now parse fields within [row_start, scan)
		size_t field_count = 0;
		size_t i = row_start;

		// If row is empty (row_start == scan), skip it
		if (i == scan) {
			continue;
		}

		// Store first field start
		state->_fields[field_count++] = &state->_data[i];

		while (i < scan) {
			if (state->_data[i] == '\t') {
				state->_data[i] = '\0';
				if (field_count < FUN_TSV_MAX_FIELDS) {
					state->_fields[field_count++] = &state->_data[i + 1];
				}
			}
			i++;
		}

		// We got a non-empty row
		row->fields = state->_fields;
		row->count = field_count;
		result.value = true;
		return result;
	}

	// End of data
	result.value = false;
	return result;
}
