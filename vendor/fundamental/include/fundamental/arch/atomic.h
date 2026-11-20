#ifndef FUNDAMENTAL_ARCH_ATOMIC_H
#define FUNDAMENTAL_ARCH_ATOMIC_H

/**
 * Fundamental Library - Atomic Operations
 * 
 * Architecture-specific atomic primitives implemented in assembly.
 * All operations provide memory ordering guarantees suitable for
 * multi-threaded synchronization.
 */

/**
 * Atomically fetch and add
 * 
 * Performs: *ptr += addend
 * Returns: value of *ptr before the addition
 * 
 * @param ptr       Pointer to integer to modify (must be aligned)
 * @param addend    Value to add
 * @return          Value at ptr before addition
 */
int fun_atomic_fetch_and_add(int *ptr, int addend);

/**
 * Atomically compare and swap (CAS)
 * 
 * Performs: if (*ptr == *expected) { *ptr = desired; return 1; }
 *           else { *expected = *ptr; return 0; }
 * 
 * @param ptr       Pointer to integer to modify (must be aligned)
 * @param expected  Pointer to expected value (updated on failure)
 * @param desired   Value to set if current value matches expected
 * @return          1 if swap succeeded, 0 otherwise
 */
int fun_atomic_compare_and_swap(int *ptr, int *expected, int desired);

/**
 * Atomic load with acquire semantics
 * 
 * @param ptr   Pointer to integer to read (must be aligned)
 * @return      Value at ptr
 */
int fun_atomic_load(int *ptr);

/**
 * Atomic store with release semantics
 * 
 * @param ptr   Pointer to integer to write (must be aligned)
 * @param value Value to store
 */
void fun_atomic_store(int *ptr, int value);

#endif /* FUNDAMENTAL_ARCH_ATOMIC_H */