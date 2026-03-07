#include "memory/memory.h"
#include "string/string.h"

void fun_string_template(String template, StringTemplateParam *params,
						 size_t paramCount, OutputString output)
{
	if (!template || !output)
		return;

	OutputString out = output;
	const char *t = template;

	while (*t) {
		// Look for template patterns
		if (*t == '$' || *t == '#' || *t == '%' || *t == '*') {
			char type = *t;
			if (*(t + 1) == '{') {
				const char *end = t + 2;
				while (*end && *end != '}')
					end++;

				if (*end == '}') {
					// Extract key name
					size_t key_len = end - (t + 2);
					char key[64];
					for (size_t i = 0; i < key_len; i++) {
						key[i] = t[2 + i];
					}
					key[key_len] = '\0';

					// Find matching parameter
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

					// Format based on type prefix
					if (found) {
						char buffer[256];
						switch (type) {
						case '$': // String
							fun_string_copy(found->value.stringValue, buffer);
							break;
						case '#': // Integer
							fun_string_from_int(found->value.intValue, 10,
												buffer);
							break;
						case '%': // Double
							fun_string_from_double(found->value.doubleValue, 3,
												   buffer);
							break;
						case '*': // Pointer
							fun_string_from_pointer(found->value.pointerValue,
													buffer);
							break;
						}

						// Copy formatted value to output
						char *b = buffer;
						while (*b)
							*out++ = *b++;
					}

					t = end + 1;
					continue;
				}
			}
		}

		// Copy regular characters
		*out++ = *t++;
	}

	*out = '\0';
}
