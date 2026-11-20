#include <stdint.h>
#include <windows.h>

uint64_t fun_timing_now_ns(void)
{
	static LARGE_INTEGER freq = { .QuadPart = 0 };
	if (freq.QuadPart == 0)
		QueryPerformanceFrequency(&freq);
	LARGE_INTEGER counter;
	QueryPerformanceCounter(&counter);
	uint64_t secs = (uint64_t)counter.QuadPart / (uint64_t)freq.QuadPart;
	uint64_t rem = (uint64_t)counter.QuadPart % (uint64_t)freq.QuadPart;
	return secs * 1000000000ULL + rem * 1000000000ULL / (uint64_t)freq.QuadPart;
}
