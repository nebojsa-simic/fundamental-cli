/* Fundamental Library - Atomic Operations
 * Linux x86_64 implementation using inline assembly
 */

#include "fundamental/arch/atomic.h"

/**
 * Atomically fetch and add
 * 
 * @param ptr       Pointer to integer to modify
 * @param addend    Value to add
 * @return          Value before addition
 */
int fun_atomic_fetch_and_add(int *ptr, int addend)
{
	int result = addend;

	__asm__ __volatile__("lock; xaddl %0, %1"
						 : "+r"(result), "+m"(*ptr)
						 :
						 : "memory", "cc");

	return result;
}

/**
 * Atomically compare and swap
 * 
 * @param ptr       Pointer to integer to modify
 * @param expected  Expected value (updated on failure)
 * @param desired   Value to set if matches expected
 * @return          1 if swap succeeded, 0 otherwise
 */
int fun_atomic_compare_and_swap(int *ptr, int *expected, int desired)
{
	int success = 0;
	int prev = *expected;

	__asm__ __volatile__("lock; cmpxchgl %3, %4"
						 : "=a"(prev), "=m"(*ptr), "=q"(success)
						 : "r"(desired), "m"(*ptr), "a"(prev)
						 : "memory", "cc");

	*expected = prev;
	return success;
}

/**
 * Atomic load with acquire semantics
 * 
 * @param ptr   Pointer to integer to read
 * @return      Value at ptr
 */
int fun_atomic_load(int *ptr)
{
	int value;

	__asm__ __volatile__("movl %1, %0" : "=r"(value) : "m"(*ptr) : "memory");

	return value;
}

/**
 * Atomic store with release semantics
 * 
 * @param ptr   Pointer to integer to write
 * @param value Value to store
 */
void fun_atomic_store(int *ptr, int value)
{
	__asm__ __volatile__("movl %1, %0" : "=m"(*ptr) : "r"(value) : "memory");
}