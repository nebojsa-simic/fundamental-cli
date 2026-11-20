#ifndef LIBRARY_STARTUP_H
#define LIBRARY_STARTUP_H

#include "../error/error.h"

/*
 * Startup Module - Centralized Initialization Framework
 *
 * Provides ordered initialization phases for library modules.
 * All initialization runs via fun_startup_run() called from __main().
 *
 * Initialization Order:
 *   Phase 1: Platform  - CPU detection, basic platform info
 *   Phase 2: Memory    - Allocator initialization
 *   Phase 3: Filesystem - Path handling, separators
 *   Phase 4: Config    - Load fun.ini configuration
 *   Phase 5: Logging   - Initialize logging (uses config)
 *   Phase 6: Network   - Initialize network (uses config)
 *   Phase 7+: Other modules
 *
 * Usage:
 *   Modules register init functions via FUNDAMENTAL_STARTUP_REGISTER():
 *
 *   // In your module source file:
 *   #include "fundamental/startup/startup.h"
 *
 *   static void my_module_init(void) {
 *       // Module initialization code
 *   }
 *
 *   FUNDAMENTAL_STARTUP_REGISTER(STARTUP_PHASE_NETWORK, my_module_init);
 *
 * Circular Dependency Prevention:
 *   - Config (phase 4) MUST NOT call logging functions
 *   - Logging (phase 5) can safely use config
 *   - Network (phase 6) can safely use config and logging
 *
 * Verbose Mode:
 *   Compile with -DFUNDAMENTAL_STARTUP_VERBOSE=1 to see phase execution.
 *
 * Example: Adding a Module to Startup Sequence
 *   --------------------------------------------
 *   // In my_module.c:
 *   #include "fundamental/startup/startup.h"
 *   #include "fundamental/console/console.h"
 *
 *   static void my_module_init(void)
 *   {
 *       // This runs at phase 7 (after platform, memory, filesystem,
 *       // config, logging, and network are initialized)
 *       fun_console_write_line("My module initialized!");
 *   }
 *
 *   // Register at phase 7 (other modules)
 *   FUNDAMENTAL_STARTUP_REGISTER(STARTUP_PHASE_OTHER, my_module_init);
 *
 *   // That's it! my_module_init() will be called automatically
 *   // when the program starts, in the correct order.
 */

/* ------------------------------------------------------------------
 * Phase Constants - Execution Order
 * ------------------------------------------------------------------ */

#define STARTUP_PHASE_PLATFORM 1
#define STARTUP_PHASE_MEMORY 2
#define STARTUP_PHASE_FILESYSTEM 3
#define STARTUP_PHASE_CONFIG 4
#define STARTUP_PHASE_LOGGING 5
#define STARTUP_PHASE_NETWORK 6
#define STARTUP_PHASE_OTHER 7

/* ------------------------------------------------------------------
 * Verbose Tracing Macro
 * ------------------------------------------------------------------ */

#if FUNDAMENTAL_STARTUP_VERBOSE
#define STARTUP_TRACE(name) fun_console_write_line("STARTUP: " name);
#else
#define STARTUP_TRACE(name) ((void)0)
#endif

/* ------------------------------------------------------------------
 * Module Registration Macro
 *
 * Modules use this to register their init function at a specific phase.
 * The macro creates a static array that the startup dispatcher reads.
 * ------------------------------------------------------------------ */

#define FUNDAMENTAL_STARTUP_REGISTER(phase, function) \
	static void (*const _startup_fn_##function)(void) \
		__attribute__((used)) = function

/* ------------------------------------------------------------------
 * Core Startup Functions
 * ------------------------------------------------------------------ */

/*
 * Run all startup initialization phases.
 *
 * Called automatically by __main() at program startup.
 * Executes phases 1-7 in order with fail-fast error handling.
 *
 * Example:
 *   // Normally called automatically, but can be called manually:
 *   fun_startup_run();
 */
void fun_startup_run(void);

/*
 * Platform initialization (Phase 1)
 *
 * Initializes platform detection (OS, architecture).
 * Silent operation - no logging, no config access.
 *
 * @return 0 on success, non-zero on failure
 */
int fun_platform_init(void);

/*
 * Memory initialization (Phase 2)
 *
 * Initializes memory allocator subsystem.
 * Silent operation - no logging.
 *
 * @return 0 on success, non-zero on failure
 */
int fun_memory_init(void);

/*
 * Filesystem initialization (Phase 3)
 *
 * Initializes filesystem path handling.
 * Silent operation - no logging.
 *
 * @return 0 on success, non-zero on failure
 */
int fun_filesystem_init(void);

/*
 * Config initialization (Phase 4)
 *
 * Loads configuration from fun.ini.
 * Silent operation - MUST NOT call logging.
 *
 * @return 0 on success, non-zero on failure
 */
int fun_config_init(void);

/*
 * Logging initialization (Phase 5)
 *
 * Initializes logging subsystem using already-loaded config.
 *
 * @return 0 on success, non-zero on failure
 */
int fun_logging_init(void);

/*
 * Network initialization (Phase 6)
 *
 * Initializes network subsystem using config for rx_buf_size.
 *
 * @return 0 on success, non-zero on failure
 */
int fun_network_init(void);

#endif /* LIBRARY_STARTUP_H */
