#include <time.h>

unsigned long long arch_async_now_ms(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (unsigned long long)ts.tv_sec * 1000ULL +
		   (unsigned long long)ts.tv_nsec / 1000000ULL;
}
