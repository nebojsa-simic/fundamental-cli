#pragma once
#include <stdint.h>

/**
 * @file shutdown.h
 * @brief Graceful shutdown framework with cleanup handlers
 * 
 * Provides coordinated shutdown with cleanup functions that execute
 * in reverse phase order when the program receives termination signals.
 * 
 * ## Usage
 * 
 * ```c
 * static void app_cleanup(void) {
 *     // Cleanup code here
 * }
 * 
 * int main(void) {
 *     // Register BEFORE fun_startup_run()
 *     shutdown_register_cleanup(SHUTDOWN_PHASE_APP, app_cleanup);
 *     fun_startup_run();  // Installs signal handlers
 *     // ... program logic
 * }
 * ```
 * 
 * ## Shutdown Phases
 * 
 * Cleanup functions execute in reverse order (highest to lowest):
 * - APP (99) → NETWORK (5) → CONFIG (4) → FILESYSTEM (3) → MEMORY (2) → PLATFORM (1)
 * 
 * ## Signal Handling
 * 
 * - Windows: Ctrl+C, Ctrl+Break via SetConsoleCtrlHandler()
 * - Linux: SIGINT, SIGTERM via signal()
 * 
 * ## Exit Codes
 * 
 * - Normal exit: 0
 * - Ctrl+C/SIGINT: 130 (128 + 2)
 * - SIGTERM: 0
 * - Emergency: 1
 */

/**
 * Shutdown type - indicates why shutdown is occurring
 */
typedef enum {
	SHUTDOWN_NORMAL = 0, /**< Normal program exit */
	SHUTDOWN_ABNORMAL = 1, /**< Crash or error */
	SHUTDOWN_EXTERNAL = 2, /**< External signal (Ctrl+C, SIGTERM) */
	SHUTDOWN_EMERGENCY = 3, /**< System shutdown/logoff */
} fun_shutdown_type;

/** Shutdown phase constants - cleanup executes in reverse order */
#define SHUTDOWN_PHASE_PLATFORM \
	1 /**< Platform layer cleanup (first to init, last to cleanup) */
#define SHUTDOWN_PHASE_MEMORY 2 /**< Memory layer cleanup */
#define SHUTDOWN_PHASE_FILESYSTEM 3 /**< Filesystem layer cleanup */
#define SHUTDOWN_PHASE_CONFIG 4 /**< Configuration layer cleanup */
#define SHUTDOWN_PHASE_NETWORK 5 /**< Network layer cleanup */
#define SHUTDOWN_PHASE_APP \
	99 /**< Application cleanup (last to init, first to cleanup) */

/**
 * Run shutdown sequence
 * 
 * Executes all registered cleanup functions in reverse phase order,
 * then terminates the process via platform_shutdown_exit().
 * 
 * This function does not return.
 * 
 * @param type Shutdown type (NORMAL, EXTERNAL, EMERGENCY, etc.)
 * @param exit_code Exit code to pass to OS
 */
void fun_shutdown_run(fun_shutdown_type type, int exit_code);

/**
 * Register a cleanup function for shutdown
 * 
 * The handler will be called during shutdown sequence, in reverse phase order.
 * Must be called BEFORE fun_startup_run() to ensure signal handlers are installed.
 * 
 * @param phase Shutdown phase constant (SHUTDOWN_PHASE_*)
 * @param handler Cleanup function to call (no parameters, no return)
 * 
 * @note Maximum 32 cleanup functions can be registered
 * @note Handlers should be fast and non-blocking
 * @note Avoid memory allocation in cleanup handlers
 */
void shutdown_register_cleanup(int phase, void (*handler)(void));

/**
 * Platform-specific shutdown exit function
 * 
 * Implemented in arch/signals/<platform>/signal.c
 * Called after all cleanup functions have executed.
 * Does not return.
 * 
 * @param exit_code Exit code to pass to OS
 */
extern void platform_shutdown_exit(int exit_code);
