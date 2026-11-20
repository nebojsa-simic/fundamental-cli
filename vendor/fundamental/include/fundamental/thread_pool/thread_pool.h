#ifndef LIBRARY_THREAD_POOL_H
#define LIBRARY_THREAD_POOL_H

#include <stddef.h>
#include <stdint.h>

#include "../error/error.h"

struct ThreadPool_s;
typedef struct ThreadPool_s *ThreadPool;

typedef struct {
	void *data;
	size_t data_size;
	void (*work_fn)(void *);
} WorkItem;

CanReturnError(void)
	fun_thread_pool_create(int32_t num_threads, ThreadPool *out_pool);
CanReturnError(void)
	fun_thread_pool_submit(ThreadPool pool, const WorkItem *item);
CanReturnError(void) fun_thread_pool_destroy(ThreadPool pool);

#endif
