#include "fundamental/math/math.h"

void _math_set_features(int sse2, int avx, int avx2, int avx512f);

static void do_cpuid(int leaf, int subleaf, int *eax, int *ebx, int *ecx,
					 int *edx)
{
	__asm__ __volatile__("cpuid"
						 : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
						 : "a"(leaf), "c"(subleaf));
}

static int has_os_support(unsigned long long mask)
{
	unsigned int eax, edx;
	__asm__ __volatile__("xgetbv" : "=a"(eax), "=d"(edx) : "c"(0));
	return ((unsigned long long)edx << 32 | eax & mask) == mask;
}

void fun_math_init(void)
{
	int eax, ebx, ecx, edx;
	int sse2 = 0, avx = 0, avx2 = 0, avx512f = 0;

	do_cpuid(1, 0, &eax, &ebx, &ecx, &edx);
	sse2 = (edx & (1 << 26)) != 0;
	avx = (ecx & (1 << 28)) != 0;

	int osxsave = (ecx & (1 << 27)) != 0;
	if (osxsave && has_os_support(6)) {
		do_cpuid(7, 0, &eax, &ebx, &ecx, &edx);
		avx2 = (ebx & (1 << 5)) != 0;
		if (has_os_support(0xE6))
			avx512f = (ebx & (1 << 16)) != 0;
	}

	_math_set_features(sse2, avx, avx2, avx512f);
}
