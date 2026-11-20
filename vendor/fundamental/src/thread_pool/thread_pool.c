#include "fundamental/thread_pool/thread_pool.h"
#include "fundamental/memory/memory.h"
#include "fundamental/sync/sync.h"

extern int arch_thread_create(void (*fn)(void *), void *arg, void **out_handle);
extern void arch_thread_join(void *handle);

typedef struct WorkerSlot_s {
	void *data;
	size_t data_size;
	void (*work_fn)(void *);
	Mutex mutex;
	CondVar condvar;
	struct ThreadPool_s *pool;
} WorkerSlot;

struct ThreadPool_s {
	WorkerSlot *slots;
	void **thread_handles;
	int32_t num_threads;
	volatile int32_t next_hint;
	volatile bool stop;
};

static void worker_loop(void *arg)
{
	WorkerSlot *slot = (WorkerSlot *)arg;
	struct ThreadPool_s *pool = slot->pool;

	for (;;) {
		fun_mutex_lock(slot->mutex);

		while (slot->data == NULL) {
			if (pool->stop) {
				fun_mutex_unlock(slot->mutex);
				return;
			}
			fun_condvar_wait(slot->condvar, slot->mutex);
		}

		void *data = slot->data;
		void (*fn)(void *) = slot->work_fn;
		slot->data = NULL;

		fun_mutex_unlock(slot->mutex);

		fn(data);

		fun_memory_free((Memory *)&data);
	}
}

CanReturnError(void)
	fun_thread_pool_create(int32_t num_threads, ThreadPool *out_pool)
{
	voidResult result;

	if (out_pool == NULL) {
		result.error = ERROR_RESULT_NULL_POINTER;
		return result;
	}

	if (num_threads <= 0) {
		result.error = ERROR_RESULT_THREAD_POOL_INVALID_SIZE;
		return result;
	}

	MemoryResult pool_mem = fun_memory_allocate(sizeof(struct ThreadPool_s));
	if (fun_error_is_error(pool_mem.error)) {
		result.error = pool_mem.error;
		return result;
	}
	struct ThreadPool_s *pool = (struct ThreadPool_s *)pool_mem.value;

	pool->num_threads = num_threads;
	pool->next_hint = 0;
	pool->stop = false;

	MemoryResult slots_mem =
		fun_memory_allocate((size_t)num_threads * sizeof(WorkerSlot));
	if (fun_error_is_error(slots_mem.error)) {
		fun_memory_free((Memory *)&pool_mem.value);
		result.error = slots_mem.error;
		return result;
	}
	pool->slots = (WorkerSlot *)slots_mem.value;

	MemoryResult handles_mem =
		fun_memory_allocate((size_t)num_threads * sizeof(void *));
	if (fun_error_is_error(handles_mem.error)) {
		fun_memory_free((Memory *)&pool->slots);
		fun_memory_free((Memory *)&pool_mem.value);
		result.error = handles_mem.error;
		return result;
	}
	pool->thread_handles = (void **)handles_mem.value;

	for (int32_t i = 0; i < num_threads; i++) {
		pool->slots[i].data = NULL;
		pool->slots[i].data_size = 0;
		pool->slots[i].work_fn = NULL;
		pool->slots[i].pool = pool;
		pool->thread_handles[i] = NULL;

		voidResult mutex_r = fun_mutex_create(&pool->slots[i].mutex);
		if (fun_error_is_error(mutex_r.error)) {
			for (int32_t j = 0; j < i; j++) {
				fun_condvar_destroy(pool->slots[j].condvar);
				fun_mutex_destroy(pool->slots[j].mutex);
			}
			fun_memory_free((Memory *)&pool->thread_handles);
			fun_memory_free((Memory *)&pool->slots);
			fun_memory_free((Memory *)&pool_mem.value);
			result.error = mutex_r.error;
			return result;
		}

		voidResult cv_r = fun_condvar_create(&pool->slots[i].condvar);
		if (fun_error_is_error(cv_r.error)) {
			fun_mutex_destroy(pool->slots[i].mutex);
			for (int32_t j = 0; j < i; j++) {
				fun_condvar_destroy(pool->slots[j].condvar);
				fun_mutex_destroy(pool->slots[j].mutex);
			}
			fun_memory_free((Memory *)&pool->thread_handles);
			fun_memory_free((Memory *)&pool->slots);
			fun_memory_free((Memory *)&pool_mem.value);
			result.error = cv_r.error;
			return result;
		}
	}

	for (int32_t i = 0; i < num_threads; i++) {
		int ret = arch_thread_create(worker_loop, &pool->slots[i],
									 &pool->thread_handles[i]);
		if (ret != 0) {
			pool->stop = true;

			for (int32_t j = 0; j < i; j++) {
				fun_condvar_broadcast(pool->slots[j].condvar);
			}

			for (int32_t j = 0; j < i; j++) {
				arch_thread_join(pool->thread_handles[j]);
			}

			for (int32_t j = 0; j < num_threads; j++) {
				fun_condvar_destroy(pool->slots[j].condvar);
				fun_mutex_destroy(pool->slots[j].mutex);
			}
			fun_memory_free((Memory *)&pool->thread_handles);
			fun_memory_free((Memory *)&pool->slots);
			fun_memory_free((Memory *)&pool_mem.value);
			result.error = ERROR_RESULT_THREAD_POOL_CREATE_FAILED;
			return result;
		}
	}

	*out_pool = (ThreadPool)pool;
	result.error = ERROR_RESULT_NO_ERROR;
	return result;
}

CanReturnError(void)
	fun_thread_pool_submit(ThreadPool pool, const WorkItem *item)
{
	voidResult result;

	if (pool == NULL) {
		result.error = ERROR_RESULT_NULL_POINTER;
		return result;
	}
	if (item == NULL) {
		result.error = ERROR_RESULT_NULL_POINTER;
		return result;
	}
	if (item->data == NULL) {
		result.error = ERROR_RESULT_NULL_POINTER;
		return result;
	}
	if (item->data_size == 0) {
		result.error = ERROR_RESULT_NULL_POINTER;
		return result;
	}
	if (item->work_fn == NULL) {
		result.error = ERROR_RESULT_NULL_POINTER;
		return result;
	}

	struct ThreadPool_s *p = (struct ThreadPool_s *)pool;

	MemoryResult copy_mem = fun_memory_allocate(item->data_size);
	if (fun_error_is_error(copy_mem.error)) {
		result.error = copy_mem.error;
		return result;
	}

	voidResult copy_result =
		fun_memory_copy(item->data, copy_mem.value, item->data_size);
	if (fun_error_is_error(copy_result.error)) {
		fun_memory_free((Memory *)&copy_mem.value);
		result.error = copy_result.error;
		return result;
	}

	int32_t hint = __atomic_load_n(&p->next_hint, __ATOMIC_RELAXED);

	for (int32_t offset = 0; offset < p->num_threads; offset++) {
		int32_t i = (hint + offset) % p->num_threads;

		fun_mutex_lock(p->slots[i].mutex);

		if (p->slots[i].data == NULL) {
			p->slots[i].data = copy_mem.value;
			p->slots[i].data_size = item->data_size;
			p->slots[i].work_fn = item->work_fn;

			__atomic_store_n(&p->next_hint, (i + 1) % p->num_threads,
							 __ATOMIC_RELAXED);

			fun_condvar_signal(p->slots[i].condvar);
			fun_mutex_unlock(p->slots[i].mutex);

			result.error = ERROR_RESULT_NO_ERROR;
			return result;
		}

		fun_mutex_unlock(p->slots[i].mutex);
	}

	fun_memory_free((Memory *)&copy_mem.value);
	result.error = ERROR_RESULT_THREAD_POOL_FULL;
	return result;
}

CanReturnError(void) fun_thread_pool_destroy(ThreadPool pool)
{
	voidResult result;

	if (pool == NULL) {
		result.error = ERROR_RESULT_NO_ERROR;
		return result;
	}

	struct ThreadPool_s *p = (struct ThreadPool_s *)pool;

	p->stop = true;

	for (int32_t i = 0; i < p->num_threads; i++) {
		fun_condvar_broadcast(p->slots[i].condvar);
	}

	for (int32_t i = 0; i < p->num_threads; i++) {
		arch_thread_join(p->thread_handles[i]);
	}

	for (int32_t i = 0; i < p->num_threads; i++) {
		fun_condvar_destroy(p->slots[i].condvar);
		fun_mutex_destroy(p->slots[i].mutex);
	}

	fun_memory_free((Memory *)&p->thread_handles);
	fun_memory_free((Memory *)&p->slots);
	fun_memory_free((Memory *)&pool);

	result.error = ERROR_RESULT_NO_ERROR;
	return result;
}
