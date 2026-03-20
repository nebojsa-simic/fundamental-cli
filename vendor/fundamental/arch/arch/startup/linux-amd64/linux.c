#include "fundamental/memory/memory.h"

#include <stdint.h>
#include <stddef.h>

#include <stddef.h>

typedef long ssize_t;

static int g_argc = 0;
static char *g_argv[64];

static inline long syscall1(long n, long a1)
{
	long ret;
	__asm__ __volatile__("syscall"
						 : "=a"(ret)
						 : "a"(n), "D"(a1)
						 : "rcx", "r11", "memory");
	return ret;
}

static inline long syscall3(long n, long a1, long a2, long a3)
{
	long ret;
	__asm__ __volatile__("syscall"
						 : "=a"(ret)
						 : "a"(n), "D"(a1), "S"(a2), "d"(a3)
						 : "rcx", "r11", "memory");
	return ret;
}

#define SYS_write 1
#define SYS_exit 60

static void write_str(const char *s, size_t len)
{
	syscall3(SYS_write, 1, (long)s, len);
}

static void exit_process(int code)
{
	syscall1(SYS_exit, code);
	__builtin_unreachable();
}

void _start(void)
{
	extern int cli_main(int argc, const char **argv);
	int result = cli_main(g_argc, (const char **)g_argv);
	exit_process(result);
}