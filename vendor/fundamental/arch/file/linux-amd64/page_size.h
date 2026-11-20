#pragma once
#include <unistd.h>
#include <stdint.h>

/*
 * Returns the system page size, queried via sysconf(_SC_PAGESIZE).
 * Falls back to 4096 if sysconf fails.  Result is cached after the
 * first call so repeated calls are cheap.
 */
static inline uint64_t get_page_size(void)
{
	static uint64_t cached = 0;
	if (cached == 0) {
		long sz = sysconf(_SC_PAGESIZE);
		cached = (sz > 0) ? (uint64_t)sz : 4096ULL;
	}
	return cached;
}
