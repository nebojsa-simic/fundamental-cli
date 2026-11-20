/* Fundamental Library - Atomic Operations
 * Windows x86_64 implementation using inline assembly
 */

#include "fundamental/arch/atomic.h"

/**
 * Atomically fetch and add
 * 
 * Uses LOCK XADDL instruction for atomic read-modify-write
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
 * Atomically compare and swap (CAS)
 * 
 * Uses LOCK CMPXCHGL instruction
 * Returns 1 on success, 0 on failure
 */
int fun_atomic_compare_and_swap(int *ptr, int *expected, int desired)
{
	int prev = *expected;
	int success = 0;

	__asm__ __volatile__("lock; cmpxchgl %3, %4\n\t"
						 "setz %0"
						 : "=q"(success), "+a"(prev), "+m"(*ptr)
						 : "r"(desired), "m"(*ptr)
						 : "memory", "cc");

	*expected = prev;
	return success;
}

/**
 * Atomic load with acquire semantics
 * 
 * Uses MOV with memory barrier
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
 * Uses MOV with memory barrier
 */
void fun_atomic_store(int *ptr, int value)
{
	__asm__ __volatile__("movl %1, %0" : "=m"(*ptr) : "r"(value) : "memory");
}