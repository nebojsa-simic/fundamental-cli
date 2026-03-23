#ifndef BUILD_GENERATOR_H
#define BUILD_GENERATOR_H

#include "build/build.h"

/**
 * Read project name from fun.ini into output buffer.
 * Returns 1 on success, 0 if absent or field not found.
 */
int build_ini_read_name(char *output, size_t output_size);

/**
 * Build space-separated source file list for GCC command.
 */
void build_sources_string(SourceScanResult scan_result, char *buffer,
						  size_t buffer_size);

/**
 * Append string literal to ptr, return new ptr.
 */
char *build_append(char *ptr, const char *s, char *buf_end);

/**
 * Write script content to file, overwriting if it exists.
 */
BuildGenerationResult build_write_script(const char *script_path,
										 const char *content);

#endif /* BUILD_GENERATOR_H */
