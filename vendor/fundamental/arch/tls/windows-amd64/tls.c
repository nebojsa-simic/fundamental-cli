#include "fundamental/arch/tls.h"
#include <windows.h>

static INIT_ONCE g_tls_init_once = INIT_ONCE_STATIC_INIT;
static DWORD g_tls_index = TLS_OUT_OF_INDEXES;

static BOOL CALLBACK tls_init_callback(PINIT_ONCE init_once, PVOID parameter,
									   PVOID *context)
{
	(void)init_once;
	(void)parameter;
	(void)context;
	g_tls_index = TlsAlloc();
	return g_tls_index != TLS_OUT_OF_INDEXES;
}

uint64_t fun_tls_u64_get(void)
{
	InitOnceExecuteOnce(&g_tls_init_once, tls_init_callback, NULL, NULL);
	if (g_tls_index == TLS_OUT_OF_INDEXES)
		return 0;
	return (uint64_t)(uintptr_t)TlsGetValue(g_tls_index);
}

void fun_tls_u64_set(uint64_t value)
{
	InitOnceExecuteOnce(&g_tls_init_once, tls_init_callback, NULL, NULL);
	if (g_tls_index == TLS_OUT_OF_INDEXES)
		return;
	TlsSetValue(g_tls_index, (void *)(uintptr_t)value);
}
