#include "fundamental/arch/tls.h"

static __thread uint64_t g_tls_u64 = 0;

uint64_t fun_tls_u64_get(void)
{
	return g_tls_u64;
}

void fun_tls_u64_set(uint64_t value)
{
	g_tls_u64 = value;
}
