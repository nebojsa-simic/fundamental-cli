#include "string/string.h"
#include "error/error.h"

CanReturnError(void) fun_string_is_valid(String source, StringLength maxLength)
{
	if (source == NULL) {
		return (voidResult){ ERROR_RESULT_NULL_POINTER };
	}

	StringLength length = 0;
	while (length < maxLength && source[length] != '\0') {
		length++;
	}

	if (length == maxLength) {
		return (voidResult){ fun_error_result(
			2, "given string is not a valid NULL terminated C string") };
	}

	return (voidResult){ ERROR_RESULT_NO_ERROR };
}
