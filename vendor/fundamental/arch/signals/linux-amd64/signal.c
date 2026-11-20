/*
 * Linux Signal Handler
 * Implements SIGTERM, SIGINT handling for graceful shutdown
 * using raw syscalls (rt_sigaction, exit_group) - no libc.
 */

#include "fundamental/shutdown/shutdown.h"

#define SYS_rt_sigaction 13
#define SYS_exit_group 231

#define SIGTERM 15
#define SIGINT 2

struct sigaction {
	void (*sa_handler)(int);
	unsigned long sa_flags;
	void (*sa_restorer)(void);
	unsigned long sa_mask;
};

static inline long syscall1(long n, long a1)
{
	long ret;
	__asm__ __volatile__("syscall"
						 : "=a"(ret)
						 : "a"(n), "D"(a1)
						 : "rcx", "r11", "memory");
	return ret;
}

static inline long syscall4(long n, long a1, long a2, long a3, long a4)
{
	long ret;
	register long r10 __asm__("r10") = a4;
	__asm__ __volatile__("syscall"
						 : "=a"(ret)
						 : "a"(n), "D"(a1), "S"(a2), "d"(a3), "r"(r10)
						 : "rcx", "r11", "memory");
	return ret;
}

/*
 * POSIX Signal Handler
 * Called on SIGTERM, SIGINT, etc.
 */
static void posix_signal_handler(int signum)
{
	switch (signum) {
	case SIGTERM:
		/* External termination request */
		fun_shutdown_run(SHUTDOWN_EXTERNAL, 0);
		break;

	case SIGINT:
		/* Interrupt from keyboard (Ctrl+C) */
		fun_shutdown_run(SHUTDOWN_EXTERNAL, 130); /* 128 + 2 */
		break;

	default:
		/* Unknown signal - abnormal shutdown */
		fun_shutdown_run(SHUTDOWN_ABNORMAL, 1);
		break;
	}
}

static void install_handler(int signum)
{
	struct sigaction sa;
	sa.sa_handler = posix_signal_handler;
	sa.sa_flags = 0;
	sa.sa_restorer = 0;
	sa.sa_mask = 0;
	syscall4(SYS_rt_sigaction, signum, (long)&sa, 0, 8);
}

/*
 * Install POSIX signal handlers
 * Called during startup to register signal handlers
 */
void fun_signals_install_handlers(void)
{
	install_handler(SIGTERM);
	install_handler(SIGINT);
}

/*
 * Platform-specific exit function for Linux
 * Called by shutdown framework after cleanup
 */
void platform_shutdown_exit(int exit_code)
{
	syscall1(SYS_exit_group, (long)exit_code);
}
