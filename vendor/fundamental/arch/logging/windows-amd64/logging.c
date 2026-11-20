/*
 * Logging timestamp implementation for Windows.
 *
 * Provides hybrid timestamp calculation:
 * - Startup: capture wall-clock (UTC) and monotonic time
 * - Runtime: calculate approximate UTC from monotonic offset
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "fundamental/string/string.h"

/* Windows API types */
typedef struct {
	unsigned long dwLowDateTime;
	unsigned long dwHighDateTime;
} FILETIME;

typedef struct {
	unsigned short wYear;
	unsigned short wMonth;
	unsigned short wDayOfWeek;
	unsigned short wDay;
	unsigned short wHour;
	unsigned short wMinute;
	unsigned short wSecond;
	unsigned short wMilliseconds;
} SYSTEMTIME;

/* Windows API function declarations */
__declspec(dllimport) void __stdcall
GetSystemTimePreciseAsFileTime(FILETIME *lpFileTime);
__declspec(dllimport) int __stdcall
QueryPerformanceCounter(long long *lpPerformanceCount);
__declspec(dllimport) int __stdcall
QueryPerformanceFrequency(long long *lpFrequency);
__declspec(dllimport) void __stdcall
FileTimeToSystemTime(const FILETIME *lpFileTime, SYSTEMTIME *lpSystemTime);

/* Static storage for hybrid timestamp calculation */
static uint64_t g_base_wall_sec = 0;
static uint64_t g_base_wall_nsec = 0;
static uint64_t g_base_perf_count = 0;
static long long g_perf_frequency = 0;
static bool g_timestamp_initialized = false;

/*
 * Convert FILETIME (100-nanosecond intervals since 1601-01-01) to Unix timestamp.
 * Unix epoch starts 1970-01-01, which is 11644473600 seconds after 1601-01-01.
 */
static void filetime_to_unix_timestamp(const FILETIME *ft, uint64_t *out_sec,
									   uint64_t *out_nsec)
{
	uint64_t ft64 = ((uint64_t)ft->dwHighDateTime << 32) | ft->dwLowDateTime;
	const uint64_t WINDOWS_EPOCH_OFFSET = 116444736000000000ULL;

	ft64 -= WINDOWS_EPOCH_OFFSET;
	*out_sec = ft64 / 10000000ULL;
	*out_nsec = (ft64 % 10000000ULL) * 100;
}

/*
 * Capture base timestamps at startup.
 * Called once during logging initialization.
 */
void arch_logging_timestamp_init(void)
{
	FILETIME wall_ft;
	long long perf_count;

	GetSystemTimePreciseAsFileTime(&wall_ft);
	filetime_to_unix_timestamp(&wall_ft, &g_base_wall_sec, &g_base_wall_nsec);

	QueryPerformanceFrequency(&g_perf_frequency);
	QueryPerformanceCounter(&perf_count);
	g_base_perf_count = (uint64_t)perf_count;

	g_timestamp_initialized = true;
}

/*
 * Get current approximate UTC timestamp.
 */
void arch_logging_get_timestamp(uint64_t *out_sec, uint64_t *out_nsec)
{
	long long perf_count;
	uint64_t elapsed_nsec;
	uint64_t wall_sec, wall_nsec;

	if (!g_timestamp_initialized) {
		FILETIME wall_ft;
		GetSystemTimePreciseAsFileTime(&wall_ft);
		filetime_to_unix_timestamp(&wall_ft, out_sec, out_nsec);
		return;
	}

	QueryPerformanceCounter(&perf_count);

	if (perf_count >= (long long)g_base_perf_count) {
		elapsed_nsec = (uint64_t)(perf_count - (long long)g_base_perf_count) *
					   1000000000ULL / (uint64_t)g_perf_frequency;
	} else {
		elapsed_nsec = 0;
	}

	wall_sec = g_base_wall_sec + elapsed_nsec / 1000000000ULL;
	wall_nsec = g_base_wall_nsec + (elapsed_nsec % 1000000000ULL);

	if (wall_nsec >= 1000000000ULL) {
		wall_sec++;
		wall_nsec -= 1000000000ULL;
	}

	*out_sec = wall_sec;
	*out_nsec = wall_nsec;
}

/*
 * Write a zero-padded integer to output buffer.
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

	for (size_t i = 0; i < padding; i++)
		output[i] = '0';

	for (size_t i = 0; i < len; i++)
		output[padding + i] = temp[i];

	return padding + len;
}

/*
 * Format timestamp as ISO 8601: YYYY-MM-DDTHH:MM:SS.mmmZ
 */
void arch_logging_format_timestamp(char *output, size_t output_size)
{
	uint64_t sec, nsec;
	uint32_t ms;
	SYSTEMTIME st;
	FILETIME ft;
	uint64_t ft64;
	size_t pos = 0;
	size_t written;

	if (output_size < 24) {
		output[0] = '\0';
		return;
	}

	arch_logging_get_timestamp(&sec, &nsec);
	ms = (uint32_t)(nsec / 1000000);

	/* Convert to FILETIME for SystemTime conversion */
	ft64 = sec * 10000000ULL + nsec / 100 + 116444736000000000ULL;
	ft.dwLowDateTime = (unsigned long)(ft64 & 0xFFFFFFFF);
	ft.dwHighDateTime = (unsigned long)(ft64 >> 32);
	FileTimeToSystemTime(&ft, &st);

	/* Build timestamp: YYYY-MM-DDTHH:MM:SS.mmmZ */
	written = write_padded_int(output + pos, output_size - pos, st.wYear, 4);
	pos += written;
	if (pos < output_size - 1)
		output[pos++] = '-';

	written = write_padded_int(output + pos, output_size - pos, st.wMonth, 2);
	pos += written;
	if (pos < output_size - 1)
		output[pos++] = '-';

	written = write_padded_int(output + pos, output_size - pos, st.wDay, 2);
	pos += written;
	if (pos < output_size - 1)
		output[pos++] = 'T';

	written = write_padded_int(output + pos, output_size - pos, st.wHour, 2);
	pos += written;
	if (pos < output_size - 1)
		output[pos++] = ':';

	written = write_padded_int(output + pos, output_size - pos, st.wMinute, 2);
	pos += written;
	if (pos < output_size - 1)
		output[pos++] = ':';

	written = write_padded_int(output + pos, output_size - pos, st.wSecond, 2);
	pos += written;
	if (pos < output_size - 1)
		output[pos++] = '.';

	written = write_padded_int(output + pos, output_size - pos, ms, 3);
	pos += written;
	if (pos < output_size - 1)
		output[pos++] = 'Z';

	output[pos] = '\0';
}
