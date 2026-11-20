#include "fundamental/object-pool/object-pool.h"

CanReturnError(ObjectPool)
	fun_object_pool_create(size_t elementSize, size_t capacity)
{
	ObjectPoolResult result;

	if (elementSize < sizeof(void *)) {
		result.value = (ObjectPool){ 0 };
		result.error = fun_error_result(ERROR_CODE_POOL_TOO_SMALL,
										"Element size too small for pool");
		return result;
	}

	if (capacity == 0) {
		result.value = (ObjectPool){ 0 };
		result.error =
			fun_error_result(ERROR_CODE_POOL_TOO_SMALL,
							 "Pool capacity must be greater than zero");
		return result;
	}

	size_t totalSize = elementSize * capacity;
	MemoryResult mem = fun_memory_allocate(totalSize);
	if (fun_error_is_error(mem.error)) {
		result.value = (ObjectPool){ 0 };
		result.error = mem.error;
		return result;
	}

	void *data = mem.value;
	void *next = (void *)0;

	// Build intrusive free list backwards
	for (size_t i = capacity; i > 0; i--) {
		void *slot = (char *)data + (i - 1) * elementSize;
		*(void **)slot = next;
		next = slot;
	}

	result.value.elementSize = elementSize;
	result.value.capacity = capacity;
	result.value.freeCount = capacity;
	result.value.freeList = data;
	result.value.memory = data;
	result.error = ERROR_RESULT_NO_ERROR;
	return result;
}

CanReturnError(void) fun_object_pool_destroy(ObjectPool *pool)
{
	voidResult result;
	if (pool == (ObjectPool *)0) {
		result.error = ERROR_RESULT_NO_ERROR;
		return result;
	}

	if (pool->freeCount < pool->capacity) {
		result.error = fun_error_result(
			ERROR_CODE_POOL_LEAKED, "Pool destroyed with outstanding slots");
	} else {
		result.error = ERROR_RESULT_NO_ERROR;
	}

	Memory mem = pool->memory;
	voidResult fr = fun_memory_free(&mem);
	if (fun_error_is_error(fr.error) && fun_error_is_ok(result.error)) {
		result.error = fr.error;
	}

	*pool = (ObjectPool){ 0 };
	return result;
}

CanReturnError(Memory) fun_object_pool_acquire(ObjectPool *pool)
{
	MemoryResult result;
	if (pool == (ObjectPool *)0) {
		result.value = (void *)0;
		result.error =
			fun_error_result(ERROR_CODE_POOL_INVALID_SLOT, "Null pool");
		return result;
	}

	if (pool->freeCount == 0) {
		result.value = (void *)0;
		result.error =
			fun_error_result(ERROR_CODE_POOL_EXHAUSTED, "Pool exhausted");
		return result;
	}

	void *slot = pool->freeList;
	pool->freeList = *(void **)slot;
	pool->freeCount--;

	result.value = (Memory)slot;
	result.error = ERROR_RESULT_NO_ERROR;
	return result;
}

CanReturnError(void) fun_object_pool_release(ObjectPool *pool, Memory slot)
{
	voidResult result;
	if (pool == (ObjectPool *)0) {
		result.error =
			fun_error_result(ERROR_CODE_POOL_INVALID_SLOT, "Null pool");
		return result;
	}

	if (slot == (void *)0) {
		result.error =
			fun_error_result(ERROR_CODE_POOL_INVALID_SLOT, "Null slot");
		return result;
	}

	void *start = pool->memory;
	void *end = (char *)start + pool->elementSize * pool->capacity;
	if (slot < start || slot >= end) {
		result.error = fun_error_result(ERROR_CODE_POOL_INVALID_SLOT,
										"Slot not owned by this pool");
		return result;
	}

	*(void **)slot = pool->freeList;
	pool->freeList = slot;
	pool->freeCount++;

	result.error = ERROR_RESULT_NO_ERROR;
	return result;
}

size_t fun_object_pool_free_count(const ObjectPool *pool)
{
	if (pool == (ObjectPool *)0)
		return 0;
	return pool->freeCount;
}

size_t fun_object_pool_capacity(const ObjectPool *pool)
{
	if (pool == (ObjectPool *)0)
		return 0;
	return pool->capacity;
}

size_t fun_object_pool_element_size(const ObjectPool *pool)
{
	if (pool == (ObjectPool *)0)
		return 0;
	return pool->elementSize;
}
