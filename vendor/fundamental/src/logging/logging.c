/*
 * Logging module - Centralized logging system.
 *
 * Implements:
 *   - Log levels (trace, debug, info, warn, error) with compile-time filtering
 *   - Hybrid timestamp calculation (startup reference + monotonic offset)
 *   - Console output
 *   - Configuration via fun.ini [logging] section
 *   - Template-based message formatting
 *
 * Initialized during startup phase 5 (after config is loaded).
 */

#include "fundamental/logging/logging.h"
#include "fundamental/console/console.h"

/* Platform-specific timestamp functions */
extern void arch_logging_timestamp_init(void);
extern void arch_logging_format_timestamp(char *output, size_t output_size);

/* Forward declarations */
static void logging_ensure_init(void);

/* Global logging state */
static LoggingConfig g_logging_config;
static bool g_logging_initialized = false;

/*
 * Extract filename from full path.
 */
const char *log_basename(const char *path)
{
	const char *last_slash = 0;
	const char *last_backslash = 0;
	const char *p = path;

	while (*p) {
		if (*p == '/')
			last_slash = p;
		else if (*p == '\\')
			last_backslash = p;
		p++;
	}

	if (last_slash && last_backslash) {
		return (last_slash > last_backslash) ? last_slash + 1 :
											   last_backslash + 1;
	} else if (last_slash) {
		return last_slash + 1;
	} else if (last_backslash) {
		return last_backslash + 1;
	}

	return path;
}

/*
 * Convert log level to string.
 */
const char *log_level_to_string(int level)
{
	switch (level) {
	case LOG_LEVEL_TRACE:
		return "TRACE";
	case LOG_LEVEL_DEBUG:
		return "DEBUG";
	case LOG_LEVEL_INFO:
		return "INFO";
	case LOG_LEVEL_WARN:
		return "WARN";
	case LOG_LEVEL_ERROR:
		return "ERROR";
	default:
		return "LOG";
	}
}

/*
 * Format timestamp using hybrid calculation.
 */
void log_format_timestamp(char *output, size_t output_size)
{
	arch_logging_format_timestamp(output, output_size);
}

/*
 * Internal log implementation.
 */
void log_impl(int level, String template_str, StringTemplateParam *params,
			  size_t param_count, const char *filename, int line)
{
	char timestamp[32];
	char formatted_message[FUNDAMENTAL_LOG_BUFFER_SIZE];
	char log_line[FUNDAMENTAL_LOG_BUFFER_SIZE + 64];
	size_t pos = 0;
	size_t remaining;
	voidResult vr;

	/* Ensure initialized */
	logging_ensure_init();

	/* Check runtime level filtering */
	if (level < g_logging_config.level) {
		return;
	}

	/* Format timestamp */
	log_format_timestamp(timestamp, sizeof(timestamp));

	/* Format message using template */
	if (template_str) {
		vr = fun_string_template(template_str, params, param_count,
								 formatted_message, sizeof(formatted_message));
		if (fun_error_is_error(vr.error)) {
			formatted_message[0] = '\0';
		}
	} else {
		formatted_message[0] = '\0';
	}

	/* Build complete log line: timestamp [LEVEL] filename:line message */
	vr = fun_string_copy(timestamp, log_line, sizeof(log_line));
	if (fun_error_is_error(vr.error)) {
		return;
	}
	pos = fun_string_length(log_line);
	remaining = sizeof(log_line) - pos;

	if (remaining > 1) {
		log_line[pos++] = ' ';
		log_line[pos++] = '[';
		vr = fun_string_copy(log_level_to_string(level), log_line + pos,
							 remaining - 1);
		if (fun_error_is_ok(vr.error)) {
			pos += fun_string_length(log_line + pos);
		}
		remaining = sizeof(log_line) - pos;
	}

	if (remaining > 2) {
		log_line[pos++] = ']';
		log_line[pos++] = ' ';
		remaining = sizeof(log_line) - pos;
	}

	if (remaining > 1) {
		vr = fun_string_copy(filename, log_line + pos, remaining);
		if (fun_error_is_ok(vr.error)) {
			pos += fun_string_length(log_line + pos);
		}
		remaining = sizeof(log_line) - pos;
	}
	if (remaining > 1) {
		log_line[pos++] = ':';
		remaining = sizeof(log_line) - pos;
	}
	if (remaining > 1) {
		vr = fun_string_from_int((int64_t)line, 10, log_line + pos, remaining);
		if (fun_error_is_ok(vr.error)) {
			pos += fun_string_length(log_line + pos);
		}
		remaining = sizeof(log_line) - pos;
	}

	if (remaining > 1) {
		log_line[pos++] = ' ';
		remaining = sizeof(log_line) - pos;
	}
	if (remaining > 1 && formatted_message[0] != '\0') {
		vr = fun_string_copy(formatted_message, log_line + pos, remaining);
		if (fun_error_is_ok(vr.error)) {
			pos += fun_string_length(log_line + pos);
		}
	}

	if (pos < sizeof(log_line) - 1) {
		log_line[pos++] = '\n';
	}
	log_line[pos] = '\0';

	if (g_logging_config.output_console) {
		fun_console_write_line(log_line);
	}
}

/*
 * Logging initialization - uses compile-time defaults.
 * Auto-called on first log if not explicitly initialized.
 */
static int logging_init(void)
{
	/* Default configuration (compile-time defaults) */
	g_logging_config.level = FUNDAMENTAL_LOG_LEVEL;
	g_logging_config.output_console = FUNDAMENTAL_LOG_OUTPUT_CONSOLE;
	g_logging_config.output_file = FUNDAMENTAL_LOG_OUTPUT_FILE;
	g_logging_config.buffer_size = FUNDAMENTAL_LOG_BUFFER_SIZE;

#if FUNDAMENTAL_LOG_OUTPUT_FILE
	{
		voidResult vr = fun_string_copy(FUNDAMENTAL_LOG_FILE_PATH,
										g_logging_config.file_path,
										sizeof(g_logging_config.file_path));
		(void)vr;
	}
#else
	g_logging_config.file_path[0] = '\0';
#endif

	/* Initialize timestamp subsystem */
	arch_logging_timestamp_init();

	g_logging_initialized = true;

	return 0;
}

/*
 * Ensure logging is initialized (auto-init on first use).
 */
static void logging_ensure_init(void)
{
	if (!g_logging_initialized) {
		logging_init();
	}
}

/*
 * Get current logging configuration.
 */
const LoggingConfig *fun_logging_get_config(void)
{
	logging_ensure_init();
	return &g_logging_config;
}

/*
 * Check if logging is initialized.
 */
bool fun_logging_is_initialized(void)
{
	return g_logging_initialized;
}

/*
 * Shutdown logging subsystem.
 */
void fun_logging_shutdown(void)
{
	g_logging_initialized = false;
}

/*
 * Public logging initialization.
 */
int fun_logging_init(void)
{
	logging_ensure_init();
	return 0;
}
