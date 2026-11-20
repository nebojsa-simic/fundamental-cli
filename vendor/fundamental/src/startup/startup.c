// Fundamental Library - Runtime Initialization
// Provides centralized startup initialization with ordered phases

#include "fundamental/startup/startup.h"
#include "fundamental/console/console.h"

/*
 * Phase function declarations - implemented by each module
 */
extern int fun_platform_init(void);
extern int fun_memory_init(void);
extern int fun_filesystem_init(void);
extern int fun_config_init(void);
extern int fun_logging_init(void);
extern int fun_network_init(void);

/* Signal handler installation (implemented in arch/signals/) */
extern void fun_signals_install_handlers(void);

/*
 * Memory initialization (Phase 2) - weak default implementation
 * Memory subsystem doesn't require initialization on most platforms.
 * Can be overridden by platform-specific implementation if needed.
 */
__attribute__((weak)) int fun_memory_init(void)
{
	return 0;
}

/*
 * Logging initialization (Phase 5) - stub implementation
 * Logging module not yet implemented - returns success.
 */
__attribute__((weak)) int fun_logging_init(void)
{
	return 0;
}

/*
 * Network initialization (Phase 6) - weak default implementation
 * Network subsystem is optional in many applications.
 */
__attribute__((weak)) int fun_network_init(void)
{
	return 0;
}

/*
 * Error handler - prints diagnostic and aborts
 */
static void startup_fatal(const char *message)
{
	fun_console_write_line("FATAL: ");
	fun_console_write_line(message);
	__builtin_trap();
}

/*
 * Run all startup initialization phases in order.
 * 
 * Phases execute: platform → memory → filesystem → config → logging → network
 * Fail-fast behavior: any phase failure (except config/logging) aborts startup.
 */
void fun_startup_run(void)
{
	int result;

	/* Phase 1: Platform initialization */
	STARTUP_TRACE("Platform init");
	result = fun_platform_init();
	if (result != 0) {
		startup_fatal("Platform initialization failed");
	}

	/* Phase 2: Memory initialization */
	STARTUP_TRACE("Memory init");
	result = fun_memory_init();
	if (result != 0) {
		startup_fatal("Memory initialization failed");
	}

	/* Phase 3: Filesystem initialization */
	STARTUP_TRACE("Filesystem init");
	result = fun_filesystem_init();
	if (result != 0) {
		startup_fatal("Filesystem initialization failed");
	}

	/* Phase 4: Config initialization */
	STARTUP_TRACE("Config init");
	result = fun_config_init();
	if (result != 0) {
		/* Config failure is non-fatal - will use defaults */
		STARTUP_TRACE("Config init failed - using defaults");
	}

	/* Phase 5: Logging initialization */
	STARTUP_TRACE("Logging init");
	result = fun_logging_init();
	if (result != 0) {
		/* Logging failure is non-fatal - continue without logging */
		STARTUP_TRACE("Logging init failed - continuing without logging");
	}

	/* Phase 6: Network initialization */
	STARTUP_TRACE("Network init");
	result = fun_network_init();
	if (result != 0) {
		/* Network failure is non-fatal for non-network apps */
		STARTUP_TRACE("Network init failed - continuing without network");
	}

	/* Install signal handlers after all initialization */
	STARTUP_TRACE("Installing signal handlers");
	fun_signals_install_handlers();

	STARTUP_TRACE("Startup complete");
}

/*
 * __main() - GCC runtime initialization entry point
 * 
 * Called automatically by GCC-generated code before main().
 * This is the standard mechanism for freestanding environments.
 */
void __main(void)
{
	fun_startup_run();
}
