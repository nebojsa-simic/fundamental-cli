#include "fundamental/math/math.h"

static int g_has_sse2;
static int g_has_avx;
static int g_has_avx2;
static int g_has_avx512f;

void _math_set_features(int sse2, int avx, int avx2, int avx512f)
{
	g_has_sse2 = sse2;
	g_has_avx = avx;
	g_has_avx2 = avx2;
	g_has_avx512f = avx512f;
}

__attribute__((weak)) void fun_math_init(void)
{
}

int fun_math_has_sse2(void)
{
	return g_has_sse2;
}

int fun_math_has_avx(void)
{
	return g_has_avx;
}

int fun_math_has_avx2(void)
{
	return g_has_avx2;
}

int fun_math_has_avx512f(void)
{
	return g_has_avx512f;
}
