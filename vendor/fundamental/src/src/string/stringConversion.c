#include "fundamental/memory/memory.h"
#include "fundamental/string/string.h"

voidResult fun_string_from_pointer(void *ptr, OutputString output,
								   size_t output_size)
{
	voidResult out;
	/* 0x + 16 hex digits + null = 19 bytes on 64-bit */
	size_t needed = 2 + sizeof(void *) * 2 + 1;
	if (!output || output_size < needed) {
		out.error = fun_error_result(ERROR_CODE_BUFFER_TOO_SMALL,
									 "Output buffer too small");
		return out;
	}

	uintptr_t addr = (uintptr_t)ptr;
	const char *hex = "0123456789abcdef";
	int i;

	*output++ = '0';
	*output++ = 'x';

	for (i = (sizeof(void *) * 2) - 1; i >= 0; i--) {
		*output++ = hex[(addr >> (i * 4)) & 0xf];
	}
	*output = '\0';

	out.error = ERROR_RESULT_NO_ERROR;
	return out;
}

voidResult fun_string_from_int(int64_t num, int base, OutputString output,
							   size_t output_size)
{
	voidResult out;
	if (!output || output_size == 0 || base < 2 || base > 36) {
		out.error =
			fun_error_result(ERROR_CODE_NULL_POINTER, "Invalid argument");
		return out;
	}

	/* max: 64 bits base-2 + sign + null = 67 */
	char local[67];
	char *buf = local;
	int64_t n = num;
	bool is_negative = false;

	if (n < 0) {
		is_negative = true;
		n = -n;
	}

	do {
		int digit = (int)(n % base);
		*buf++ = (digit < 10) ? (char)(digit + '0') : (char)(digit - 10 + 'a');
		n /= base;
	} while (n > 0);

	if (is_negative)
		*buf++ = '-';

	*buf = '\0';

	fun_string_reverse_in_place(local);

	StringLength len = fun_string_length(local);
	if (len + 1 > output_size) {
		out.error = fun_error_result(ERROR_CODE_BUFFER_TOO_SMALL,
									 "Output buffer too small");
		return out;
	}

	char *s = local;
	while (*s)
		*output++ = *s++;
	*output = '\0';

	out.error = ERROR_RESULT_NO_ERROR;
	return out;
}

voidResult fun_string_from_double(double num, int afterpoint,
								  OutputString output, size_t output_size)
{
	voidResult out;
	if (!output || output_size == 0 || afterpoint < 0) {
		out.error =
			fun_error_result(ERROR_CODE_NULL_POINTER, "Invalid argument");
		return out;
	}

	/* compute into a local buffer first, then check fit */
	char local[256];
	char *buf = local;

	int64_t ipart = (int64_t)num;
	double fpart = num - (double)ipart;

	if (num < 0) {
		ipart = -ipart;
		fpart = -fpart;
		*buf++ = '-';
	}

	/* convert integer part into remaining local space */
	size_t used = (size_t)(buf - local);
	voidResult ir = fun_string_from_int(ipart, 10, buf, sizeof(local) - used);
	if (fun_error_is_error(ir.error)) {
		out.error = ir.error;
		return out;
	}

	while (*buf != '\0')
		buf++;

	if (afterpoint > 0) {
		*buf++ = '.';

		while (afterpoint > 0) {
			fpart *= 10;
			int digit = (int)fpart;
			*buf++ = (char)(digit + '0');
			fpart -= digit;
			afterpoint--;
		}
	}

	*buf = '\0';

	StringLength len = fun_string_length(local);
	if (len + 1 > output_size) {
		out.error = fun_error_result(ERROR_CODE_BUFFER_TOO_SMALL,
									 "Output buffer too small");
		return out;
	}

	char *s = local;
	while (*s)
		*output++ = *s++;
	*output = '\0';

	out.error = ERROR_RESULT_NO_ERROR;
	return out;
}
