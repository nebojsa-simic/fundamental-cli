#include "memory/memory.h"
#include "string/string.h"

#define MAX_NUMBER_LENGTH 256

void fun_string_from_pointer(void *ptr, OutputString output)
{
	OutputString buf = output;

	uintptr_t addr = (uintptr_t)ptr;
	const char *hex = "0123456789abcdef";
	int i;

	*buf++ = '0';
	*buf++ = 'x';

	for (i = (sizeof(void *) * 2) - 1; i >= 0; i--) {
		*buf++ = hex[(addr >> (i * 4)) & 0xf];
	}
	*buf = '\0';
}

void fun_string_from_int(int64_t num, int base, OutputString output)
{
	if (!output || base < 2 || base > 36)
		return;

	char *buf = output;
	int64_t n = num;
	bool is_negative = false;

	if (n < 0) {
		is_negative = true;
		n = -n;
	}

	do {
		int digit = n % base;
		*buf++ = (digit < 10) ? (digit + '0') : (digit - 10 + 'a');
		n /= base;
	} while (n > 0);

	if (is_negative)
		*buf++ = '-';

	*buf = '\0';

	// Reverse the string
	fun_string_reverse_in_place(output);
}

void fun_string_from_double(double num, int afterpoint, OutputString output)
{
	if (!output || afterpoint < 0)
		return;

	char *buf = output;
	int64_t ipart = (int64_t)num;
	double fpart = num - (double)ipart;
	int is_negative = 0;

	// Handle negative numbers
	if (num < 0) {
		is_negative = 1;
		ipart = -ipart;
		fpart = -fpart;
		*buf++ = '-';
	}

	// Convert integer part
	fun_string_from_int(ipart, 10, buf);

	// Find the end of the integer part
	while (*buf != '\0')
		buf++;

	// Add decimal point if needed
	if (afterpoint > 0) {
		*buf++ = '.';

		// Convert fractional part
		while (afterpoint > 0) {
			fpart *= 10;
			int digit = (int)fpart;
			*buf++ = digit + '0';
			fpart -= digit;
			afterpoint--;
		}
	}

	*buf = '\0';
}
