#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

unsigned long long arch_async_now_ms(void)
{
	return (unsigned long long)GetTickCount64();
}
