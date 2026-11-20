#ifndef LIBRARY_OBJECT_POOL_H
#define LIBRARY_OBJECT_POOL_H

#include <stddef.h>

#include "../error/error.h"
#include "../memory/memory.h"

// Core object pool structure — fixed-size, fixed-capacity
typedef struct {
	size_t elementSize;
	size_t capacity;
	size_t freeCount;
	void *freeList;
	void *memory;
} ObjectPool;

DEFINE_RESULT_TYPE(ObjectPool);

// Error codes for object pool operations
#define ERROR_CODE_POOL_EXHAUSTED 201
#define ERROR_CODE_POOL_TOO_SMALL 202
#define ERROR_CODE_POOL_INVALID_SLOT 203
#define ERROR_CODE_POOL_LEAKED 204

// Lifecycle
CanReturnError(ObjectPool)
	fun_object_pool_create(size_t elementSize, size_t capacity);
CanReturnError(void) fun_object_pool_destroy(ObjectPool *pool);

// Slot operations — O(1), no syscalls
CanReturnError(Memory) fun_object_pool_acquire(ObjectPool *pool);
CanReturnError(void) fun_object_pool_release(ObjectPool *pool, Memory slot);

// Query
size_t fun_object_pool_free_count(const ObjectPool *pool);
size_t fun_object_pool_capacity(const ObjectPool *pool);
size_t fun_object_pool_element_size(const ObjectPool *pool);

// Macro to define type-safe pool wrapper
#define DEFINE_OBJECT_POOL_TYPE(T)                                          \
	typedef struct {                                                        \
		ObjectPool pool;                                                    \
	} T##Pool;                                                              \
                                                                            \
	static inline ObjectPoolResult fun_object_pool_##T##_create(            \
		size_t capacity)                                                    \
	{                                                                       \
		return fun_object_pool_create(sizeof(T), capacity);                 \
	}                                                                       \
                                                                            \
	static inline T *fun_object_pool_##T##_acquire(T##Pool *p)              \
	{                                                                       \
		MemoryResult r = fun_object_pool_acquire(&p->pool);                 \
		if (fun_error_is_error(r.error))                                    \
			return (T *)0;                                                  \
		return (T *)r.value;                                                \
	}                                                                       \
                                                                            \
	static inline voidResult fun_object_pool_##T##_release(T##Pool *p,      \
														   T *slot)         \
	{                                                                       \
		return fun_object_pool_release(&p->pool, (Memory)slot);             \
	}                                                                       \
                                                                            \
	static inline voidResult fun_object_pool_##T##_destroy(T##Pool *p)      \
	{                                                                       \
		return fun_object_pool_destroy(&p->pool);                           \
	}                                                                       \
                                                                            \
	static inline size_t fun_object_pool_##T##_free_count(const T##Pool *p) \
	{                                                                       \
		return fun_object_pool_free_count(&p->pool);                        \
	}                                                                       \
                                                                            \
	static inline size_t fun_object_pool_##T##_capacity(const T##Pool *p)   \
	{                                                                       \
		return fun_object_pool_capacity(&p->pool);                          \
	}

#endif // LIBRARY_OBJECT_POOL_H
