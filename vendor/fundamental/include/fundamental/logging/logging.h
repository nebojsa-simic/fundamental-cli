#ifndef LIBRARY_LOGGING_H
#define LIBRARY_LOGGING_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "../string/string.h"
#include "../error/error.h"

/*
 * Logging Module - Centralized Logging System
 *
 * Provides logging functionality with configurable log levels,
 * timestamps, and multiple output targets.
 *
 * Log Levels (ascending priority):
 *   LOG_LEVEL_TRACE - Trace level (most verbose)
 *   LOG_LEVEL_DEBUG - Debug messages
 *   LOG_LEVEL_INFO  - Informational messages
 *   LOG_LEVEL_WARN  - Warning messages
 *   LOG_LEVEL_ERROR - Error messages (highest priority)
 *
 * Usage:
 *   // Log messages at different levels using templates
 *   StringTemplateParam p[] = {
 *       {"user_id", {.intValue = 42}},
 *       {"ip", {.stringValue = "192.168.1.1"}}
 *   };
 *   log_info("User #{user_id} from ${ip} logged in", p, 2);
 *   log_error("Connection failed", NULL, 0);
 *
 * Compile-Time Configuration (define before including):
 *   #define FUNDAMENTAL_LOG_LEVEL LOG_LEVEL_INFO
 *   #define FUNDAMENTAL_LOG_OUTPUT_CONSOLE 1
 *   #define FUNDAMENTAL_LOG_OUTPUT_FILE 0
 *   #define FUNDAMENTAL_LOG_FILE_PATH "/tmp/app.log"
 *   #define FUNDAMENTAL_LOG_BUFFER_SIZE 512
 *
 * Runtime Configuration (via fun.ini [logging] section):
 *   [logging]
 *   level = INFO
 *   output_console = 1
 *   output_file = 1
 *   file_path = /var/log/myapp.log
 *   buffer_size = 1024
 *
 * IMPORTANT: Config module MUST NOT call logging functions
 * (circular dependency prevention)
 */

/* ------------------------------------------------------------------
 * Compile-Time Configuration with Defaults
 * ------------------------------------------------------------------ */

#ifndef FUNDAMENTAL_LOG_LEVEL
#define FUNDAMENTAL_LOG_LEVEL LOG_LEVEL_INFO
#endif

#ifndef FUNDAMENTAL_LOG_OUTPUT_CONSOLE
#define FUNDAMENTAL_LOG_OUTPUT_CONSOLE 1
#endif

#ifndef FUNDAMENTAL_LOG_OUTPUT_FILE
#define FUNDAMENTAL_LOG_OUTPUT_FILE 0
#endif

#ifndef FUNDAMENTAL_LOG_FILE_PATH
#define FUNDAMENTAL_LOG_FILE_PATH "/tmp/fundamental.log"
#endif

#ifndef FUNDAMENTAL_LOG_BUFFER_SIZE
#define FUNDAMENTAL_LOG_BUFFER_SIZE 512
#endif

/* ------------------------------------------------------------------
 * Log Level Constants
 * ------------------------------------------------------------------ */

#define LOG_LEVEL_TRACE 0
#define LOG_LEVEL_DEBUG 1
#define LOG_LEVEL_INFO 2
#define LOG_LEVEL_WARN 3
#define LOG_LEVEL_ERROR 4

/* ------------------------------------------------------------------
 * Logging Configuration Structure
 * ------------------------------------------------------------------ */

typedef struct {
	int level;
	bool output_console;
	bool output_file;
	char file_path[256];
	size_t buffer_size;
} LoggingConfig;

/* ------------------------------------------------------------------
 * Internal Functions (not for direct use)
 * ------------------------------------------------------------------ */

void fun_logging_shutdown(void);

int fun_logging_init(void);

void log_impl(int level, String template_str, StringTemplateParam *params,
			  size_t param_count, const char *filename, int line);

const char *log_level_to_string(int level);
const char *log_basename(const char *path);

void log_format_timestamp(char *output, size_t output_size);

/* ------------------------------------------------------------------
 * LOG_BASENAME Macro - Extract filename from path
 * ------------------------------------------------------------------ */

#define LOG_BASENAME(path) log_basename(path)

/* ------------------------------------------------------------------
 * Public Log Macros - Compile-Time Level Filtering
 *
 * Each macro compiles to ((void)0) when disabled for zero overhead.
 * ------------------------------------------------------------------ */

#if FUNDAMENTAL_LOG_LEVEL <= LOG_LEVEL_TRACE
#define log_trace(template, params, count)                                     \
	log_impl(LOG_LEVEL_TRACE, template, params, count, LOG_BASENAME(__FILE__), \
			 __LINE__)
#else
#define log_trace(template, params, count) ((void)0)
#endif

#if FUNDAMENTAL_LOG_LEVEL <= LOG_LEVEL_DEBUG
#define log_debug(template, params, count)                                     \
	log_impl(LOG_LEVEL_DEBUG, template, params, count, LOG_BASENAME(__FILE__), \
			 __LINE__)
#else
#define log_debug(template, params, count) ((void)0)
#endif

#if FUNDAMENTAL_LOG_LEVEL <= LOG_LEVEL_INFO
#define log_info(template, params, count)                                     \
	log_impl(LOG_LEVEL_INFO, template, params, count, LOG_BASENAME(__FILE__), \
			 __LINE__)
#else
#define log_info(template, params, count) ((void)0)
#endif

#if FUNDAMENTAL_LOG_LEVEL <= LOG_LEVEL_WARN
#define log_warn(template, params, count)                                     \
	log_impl(LOG_LEVEL_WARN, template, params, count, LOG_BASENAME(__FILE__), \
			 __LINE__)
#else
#define log_warn(template, params, count) ((void)0)
#endif

#if FUNDAMENTAL_LOG_LEVEL <= LOG_LEVEL_ERROR
#define log_error(template, params, count)                                     \
	log_impl(LOG_LEVEL_ERROR, template, params, count, LOG_BASENAME(__FILE__), \
			 __LINE__)
#else
#define log_error(template, params, count) ((void)0)
#endif

/* ------------------------------------------------------------------
 * Runtime Configuration Functions
 * ------------------------------------------------------------------ */

/*
 * Get current logging configuration.
 *
 * @return Pointer to current LoggingConfig (read-only)
 */
const LoggingConfig *fun_logging_get_config(void);

/*
 * Check if logging is initialized.
 *
 * @return true if initialized, false otherwise
 */
bool fun_logging_is_initialized(void);

#endif /* LIBRARY_LOGGING_H */
