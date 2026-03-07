#ifndef LIBRARY_CONSOLE_H
#define LIBRARY_CONSOLE_H

#include "../error/error.h"
#include "../string/string.h"

/**
 * Write a string to console (stdout) with automatic newline.
 * 
 * Output is line-buffered with a 512-byte buffer. The buffer is automatically
 * flushed when:
 * - A newline character is written
 * - The buffer becomes full
 * - fun_console_flush() is called explicitly
 * 
 * @param string Null-terminated string to write. Must not be NULL.
 * @return ErrorResult indicating success (ERROR_RESULT_NO_ERROR) or failure
 * 
 * Example:
 * ```c
 * fun_console_write_line("Hello, World!");
 * ```
 */
ErrorResult fun_console_write_line(String string);

/**
 * Write a string to console error (stderr) with automatic newline.
 * 
 * Stderr output is NOT buffered - it is written immediately.
 * 
 * @param string Null-terminated string to write. Must not be NULL.
 * @return ErrorResult indicating success (ERROR_RESULT_NO_ERROR) or failure
 * 
 * Example:
 * ```c
 * fun_console_error_line("An error occurred!");
 * ```
 */
ErrorResult fun_console_error_line(String string);

/**
 * Write a string to console (stdout) without automatic newline.
 * 
 * Output is buffered. Use fun_console_flush() to force output, or call
 * fun_console_write_line() to flush with newline.
 * 
 * @param string Null-terminated string to write. Must not be NULL.
 * @return ErrorResult indicating success (ERROR_RESULT_NO_ERROR) or failure
 * 
 * Example:
 * ```c
 * fun_console_write("Progress: ");
 * fun_console_write("50%%");
 * fun_console_write_line(" complete");
 * ```
 */
ErrorResult fun_console_write(String string);

/**
 * Flush console output buffer explicitly.
 * 
 * Forces any buffered output to be written to stdout immediately.
 * 
 * @return ErrorResult indicating success (ERROR_RESULT_NO_ERROR) or failure
 * 
 * Example:
 * ```c
 * fun_console_write("Processing...");
 * fun_console_flush();  // Force output now
 * ```
 */
ErrorResult fun_console_flush(void);

#endif // LIBRARY_CONSOLE_H
