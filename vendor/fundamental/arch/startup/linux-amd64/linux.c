#include "fundamental/memory/memory.h"

extern int cli_main(int argc, const char **argv);

/* Saved environment pointer — used by process spawning */
const char **fun_arch_envp;

int _start_c(long *sp)
{
	int argc = (int)*sp;
	const char **argv = (const char **)(sp + 1);
	fun_arch_envp = argv + argc + 1;
	return cli_main(argc, argv);
}

__attribute__((naked)) void _start(void)
{
	__asm__ __volatile__(
		"xor %%rbp, %%rbp\n\t"
		"mov %%rsp, %%rdi\n\t"
		"and $~15, %%rsp\n\t"
		"call _start_c\n\t"
		"mov %%eax, %%edi\n\t"
		"mov $60, %%eax\n\t"
		"syscall\n\t"
		::: "memory"
	);
}
