#include "fundamental/math/math.h"

#include <intrin.h>

void _math_set_features(int sse2, int avx, int avx2, int avx512f);

static unsigned long long _do_xgetbv(unsigned int xcr)
{
	unsigned int eax, edx;
	__asm__ __volatile__("xgetbv" : "=a"(eax), "=d"(edx) : "c"(xcr));
	return ((unsigned long long)edx << 32) | eax;
}

void fun_math_init(void)
{
	int info[4];
	int sse2 = 0, avx = 0, avx2 = 0, avx512f = 0;

	__cpuid(info, 1);
	sse2 = (info[3] & (1 << 26)) != 0;
	avx = (info[2] & (1 << 28)) != 0;

	int osxsave = (info[2] & (1 << 27)) != 0;
	if (osxsave) {
		unsigned long long xcr0 = _do_xgetbv(0);
		int os_avx = (xcr0 & 6) == 6;
		int os_avx512 = (xcr0 & 0xE6) == 0xE6;

		if (os_avx) {
			__cpuid(info, 7);
			avx2 = (info[1] & (1 << 5)) != 0;
			if (os_avx512)
				avx512f = (info[1] & (1 << 16)) != 0;
		}
	}

	_math_set_features(sse2, avx, avx2, avx512f);
}
