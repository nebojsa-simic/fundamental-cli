#ifndef FUNDAMENTAL_ARCH_TLS_H
#define FUNDAMENTAL_ARCH_TLS_H

#include <stdint.h>

/**
 * Fundamental Library - Thread-Local Storage
 *
 * Architecture-specific thread-local storage primitives that avoid the
 * compiler's emulated TLS runtime (libgcc's __emutls_get_address), which
 * some toolchains require for `_Thread_local`/`__thread` on Windows.
 */

/**
 * Read the current thread's stored 64-bit value.
 *
 * @return The per-thread value (0 if unset)
 */
uint64_t fun_tls_u64_get(void);

/**
 * Store a 64-bit value in the current thread's slot.
 *
 * @param value Value to store
 */
void fun_tls_u64_set(uint64_t value);

#endif /* FUNDAMENTAL_ARCH_TLS_H */
