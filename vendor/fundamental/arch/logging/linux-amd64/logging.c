/*
 * Logging timestamp implementation for Linux.
 *
 * Provides hybrid timestamp calculation:
 * - Startup: capture wall-clock (UTC) and monotonic time
 * - Runtime: calculate approximate UTC from monotonic offset
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "fundamental/string/string.h"

struct timespec {
	long tv_sec;
	long tv_nsec;
};

#define CLOCK_REALTIME 0
#define CLOCK_MONOTONIC 1

static long sys_clock_gettime(int clkid, struct timespec *tp)
{
	long ret;
	__asm__ __volatile__("syscall"
						 : "=a"(ret)
						 : "0"(228L), "D"((long)clkid), "S"(tp)
						 : "rcx", "r11", "memory");
	return ret;
}

/* Static storage for hybrid timestamp calculation */
static uint64_t g_base_wall_sec = 0;
static uint64_t g_base_wall_nsec = 0;
static uint64_t g_base_mono_sec = 0;
static uint64_t g_base_mono_nsec = 0;
static bool g_timestamp_initialized = false;

/*
 * Capture base timestamps at startup.
 * Called once during logging initialization.
 */
void arch_logging_timestamp_init(void)
{
	struct timespec wall, mono;

	sys_clock_gettime(CLOCK_REALTIME, &wall);
	sys_clock_gettime(CLOCK_MONOTONIC, &mono);

	g_base_wall_sec = (uint64_t)wall.tv_sec;
	g_base_wall_nsec = (uint64_t)wall.tv_nsec;
	g_base_mono_sec = (uint64_t)mono.tv_sec;
	g_base_mono_nsec = (uint64_t)mono.tv_nsec;

	g_timestamp_initialized = true;
}

/*
 * Get current approximate UTC timestamp.
 *
 * Uses hybrid calculation: base_wall_time + (now_monotonic - base_mono)
 * This avoids syscall per log call.
 *
 * @param out_sec Output: seconds since epoch
 * @param out_nsec Output: nanoseconds within second
 */
void arch_logging_get_timestamp(uint64_t *out_sec, uint64_t *out_nsec)
{
	struct timespec now_mono;
	uint64_t mono_elapsed_sec, mono_elapsed_nsec;
	uint64_t wall_sec, wall_nsec;

	if (!g_timestamp_initialized) {
		/* Fallback if not initialized */
		struct timespec now_wall;
		sys_clock_gettime(CLOCK_REALTIME, &now_wall);
		*out_sec = (uint64_t)now_wall.tv_sec;
		*out_nsec = (uint64_t)now_wall.tv_nsec;
		return;
	}

	/* Get current monotonic time */
	sys_clock_gettime(CLOCK_MONOTONIC, &now_mono);

	/* Calculate elapsed time since startup */
	mono_elapsed_sec = (uint64_t)now_mono.tv_sec - g_base_mono_sec;
	if (now_mono.tv_nsec >= (long)g_base_mono_nsec) {
		mono_elapsed_nsec = (uint64_t)now_mono.tv_nsec - g_base_mono_nsec;
	} else {
		mono_elapsed_sec--;
		mono_elapsed_nsec =
			1000000000ULL - g_base_mono_nsec + (uint64_t)now_mono.tv_nsec;
	}

	/* Add elapsed time to base wall time */
	wall_sec = g_base_wall_sec + mono_elapsed_sec;
	wall_nsec = g_base_wall_nsec + mono_elapsed_nsec;

	/* Handle nanosecond overflow */
	if (wall_nsec >= 1000000000ULL) {
		wall_sec++;
		wall_nsec -= 1000000000ULL;
	}

	*out_sec = wall_sec;
	*out_nsec = wall_nsec;
}

/*
 * Write a zero-padded integer to output buffer.
 * Returns number of characters written.
 */
static size_t write_padded_int(char *output, size_t output_size, int64_t value,
							   size_t min_width)
{
	char temp[32];
	voidResult vr;
	size_t len;
	size_t padding;

	vr = fun_string_from_int(value, 10, temp, sizeof(temp));
	if (fun_error_is_error(vr.error))
		return 0;

	len = fun_string_length(temp);
	padding = (min_width > len) ? (min_width - len) : 0;

	if (output_size < padding + len)
		return 0;

	/* Write padding zeros */
	for (size_t i = 0; i < padding; i++)
		output[i] = '0';

	/* Write number */
	for (size_t i = 0; i < len; i++)
		output[padding + i] = temp[i];

	return padding + len;
}

/*
 * Format timestamp as ISO 8601: YYYY-MM-DDTHH:MM:SS.mmmZ
 *
 * @param output Buffer to store formatted timestamp
 * @param output_size Size of output buffer (must be >= 24 bytes)
 */
void arch_logging_format_timestamp(char *output, size_t output_size)
{
	uint64_t sec, nsec;
	uint32_t ms;
	uint32_t year, month, day, hour, minute, second;
	uint64_t days, remaining;
	const uint32_t days_in_month[] = { 31, 28, 31, 30, 31, 30,
									   31, 31, 30, 31, 30, 31 };
	size_t pos = 0;
	size_t written;
	bool is_leap;

	if (output_size < 24) {
		output[0] = '\0';
		return;
	}

	arch_logging_get_timestamp(&sec, &nsec);
	ms = (uint32_t)(nsec / 1000000);

	/* Convert Unix timestamp to UTC date/time */
	days = sec / 86400;
	remaining = sec % 86400;

	hour = (uint32_t)(remaining / 3600);
	remaining %= 3600;
	minute = (uint32_t)(remaining / 60);
	second = (uint32_t)(remaining % 60);

	/* Calculate year */
	year = 1970;
	while (1) {
		uint32_t days_in_year =
			((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) ? 366 :
																		365;
		if (days < days_in_year)
			break;
		days -= days_in_year;
		year++;
	}

	/* Calculate month and day */
	is_leap = ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0));
	month = 1;
	for (uint32_t i = 0; i < 12; i++) {
		uint32_t dim = days_in_month[i];
		if (i == 1 && is_leap)
			dim = 29;
		if (days < dim) {
			day = days + 1;
			break;
		}
		days -= dim;
		month++;
	}

	/* Build timestamp: YYYY-MM-DDTHH:MM:SS.mmmZ */
	/* Year: 4 digits */
	written = write_padded_int(output + pos, output_size - pos, year, 4);
	pos += written;
	if (pos < output_size - 1)
		output[pos++] = '-';

	/* Month: 2 digits */
	written = write_padded_int(output + pos, output_size - pos, month, 2);
	pos += written;
	if (pos < output_size - 1)
		output[pos++] = '-';

	/* Day: 2 digits */
	written = write_padded_int(output + pos, output_size - pos, day, 2);
	pos += written;
	if (pos < output_size - 1)
		output[pos++] = 'T';

	/* Hour: 2 digits */
	written = write_padded_int(output + pos, output_size - pos, hour, 2);
	pos += written;
	if (pos < output_size - 1)
		output[pos++] = ':';

	/* Minute: 2 digits */
	written = write_padded_int(output + pos, output_size - pos, minute, 2);
	pos += written;
	if (pos < output_size - 1)
		output[pos++] = ':';

	/* Second: 2 digits */
	written = write_padded_int(output + pos, output_size - pos, second, 2);
	pos += written;
	if (pos < output_size - 1)
		output[pos++] = '.';

	/* Milliseconds: 3 digits */
	written = write_padded_int(output + pos, output_size - pos, ms, 3);
	pos += written;
	if (pos < output_size - 1)
		output[pos++] = 'Z';

	output[pos] = '\0';
}
