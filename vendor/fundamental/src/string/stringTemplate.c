#include "fundamental/memory/memory.h"
#include "fundamental/string/string.h"

voidResult fun_string_template(String template, StringTemplateParam *params,
							   size_t paramCount, OutputString output,
							   size_t output_size)
{
	voidResult out;
	if (!template || !output) {
		out.error = fun_error_result(ERROR_CODE_NULL_POINTER, "Null argument");
		return out;
	}

	OutputString o = output;
	size_t remaining = output_size;
	const char *t = template;

	while (*t) {
		if (*t == '$' || *t == '#' || *t == '%' || *t == '*') {
			char type = *t;
			if (*(t + 1) == '{') {
				const char *end = t + 2;
				while (*end && *end != '}')
					end++;

				if (*end == '}') {
					size_t key_len = end - (t + 2);
					char key[64];
					for (size_t i = 0; i < key_len; i++) {
						key[i] = t[2 + i];
					}
					key[key_len] = '\0';

					StringTemplateParam *found = NULL;
					for (size_t i = 0; i < paramCount; i++) {
						size_t j = 0;
						while (params[i].key[j] && key[j] &&
							   params[i].key[j] == key[j])
							j++;
						if (params[i].key[j] == '\0' && key[j] == '\0') {
							found = &params[i];
							break;
						}
					}

					if (found) {
						char buffer[256];
						voidResult cr;
						switch (type) {
						case '$':
							cr = fun_string_copy(found->value.stringValue,
												 buffer, sizeof(buffer));
							break;
						case '#':
							cr = fun_string_from_int(found->value.intValue, 10,
													 buffer, sizeof(buffer));
							break;
						case '%':
							cr = fun_string_from_double(
								found->value.doubleValue, 3, buffer,
								sizeof(buffer));
							break;
						default: /* '*' */
							cr = fun_string_from_pointer(
								found->value.pointerValue, buffer,
								sizeof(buffer));
							break;
						}

						if (fun_error_is_error(cr.error)) {
							out.error = cr.error;
							return out;
						}

						char *b = buffer;
						while (*b) {
							if (remaining < 2) {
								out.error = fun_error_result(
									ERROR_CODE_BUFFER_TOO_SMALL,
									"Output buffer too small");
								return out;
							}
							*o++ = *b++;
							remaining--;
						}
					}

					t = end + 1;
					continue;
				}
			}
		}

		if (remaining < 2) {
			out.error = fun_error_result(ERROR_CODE_BUFFER_TOO_SMALL,
										 "Output buffer too small");
			return out;
		}
		*o++ = *t++;
		remaining--;
	}

	*o = '\0';
	out.error = ERROR_RESULT_NO_ERROR;
	return out;
}
