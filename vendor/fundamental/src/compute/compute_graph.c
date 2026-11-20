#include "fundamental/compute/compute.h"
#include "fundamental/memory/memory.h"
#include "fundamental/sync/sync.h"
#include "fundamental/thread_pool/thread_pool.h"

typedef struct {
	int dependent;
	int next;
} EdgeEntry;

typedef struct {
	FunComputeTask *caller_task;
	int ndeps_total;
	int ndeps_remaining;
	int first_edge;
	int ndependents;
} TaskMeta;

typedef struct {
	FunComputeGraph graph;
} WorkerCtx;

struct FunComputeGraph_s {
	TaskMeta *meta;
	EdgeEntry *edges;
	int *ready_queue;
	int n_tasks;
	int max_tasks;
	int n_edges;
	int max_edges;
	int ready_head;
	int ready_tail;
	int ready_count;
	int pending_count;
	int workers_running;
	Mutex ready_mutex;
	CondVar ready_condvar;
	Mutex done_mutex;
	CondVar done_condvar;
	ThreadPool thread_pool;
	int n_threads;
};

static int _find_task_index(FunComputeGraph graph, FunComputeTask *task)
{
	for (int i = 0; i < graph->n_tasks; i++)
		if (graph->meta[i].caller_task == task)
			return i;
	return -1;
}

static void _enqueue_ready(FunComputeGraph graph, int idx)
{
	graph->ready_queue[graph->ready_tail++] = idx;
	graph->ready_count++;
}

static int _dequeue_ready(FunComputeGraph graph)
{
	int idx = graph->ready_queue[graph->ready_head++];
	graph->ready_count--;
	return idx;
}

static void _complete_task(FunComputeGraph graph, int idx)
{
	int e = graph->meta[idx].first_edge;
	while (e >= 0) {
		int dep = graph->edges[e].dependent;
		graph->meta[dep].ndeps_remaining--;
		if (graph->meta[dep].ndeps_remaining == 0)
			_enqueue_ready(graph, dep);
		e = graph->edges[e].next;
	}
}

static void _worker_loop(void *data)
{
	FunComputeGraph graph = ((WorkerCtx *)data)->graph;
	for (;;) {
		fun_mutex_lock(graph->ready_mutex);
		while (graph->ready_count == 0 && graph->pending_count > 0)
			fun_condvar_wait(graph->ready_condvar, graph->ready_mutex);
		if (graph->ready_count == 0 && graph->pending_count == 0) {
			fun_mutex_unlock(graph->ready_mutex);
			break;
		}
		int idx = _dequeue_ready(graph);
		fun_mutex_unlock(graph->ready_mutex);

		graph->meta[idx].caller_task->fn(graph->meta[idx].caller_task->ctx);

		fun_mutex_lock(graph->ready_mutex);
		_complete_task(graph, idx);
		fun_condvar_signal(graph->ready_condvar);
		fun_mutex_unlock(graph->ready_mutex);

		fun_mutex_lock(graph->done_mutex);
		graph->pending_count--;
		int is_done = (graph->pending_count == 0);
		fun_mutex_unlock(graph->done_mutex);

		if (is_done) {
			fun_mutex_lock(graph->ready_mutex);
			fun_condvar_broadcast(graph->ready_condvar);
			fun_mutex_unlock(graph->ready_mutex);

			fun_mutex_lock(graph->done_mutex);
			fun_condvar_signal(graph->done_condvar);
			fun_mutex_unlock(graph->done_mutex);
		}
	}

	fun_mutex_lock(graph->done_mutex);
	graph->workers_running--;
	if (graph->workers_running == 0)
		fun_condvar_signal(graph->done_condvar);
	fun_mutex_unlock(graph->done_mutex);
}

size_t fun_compute_graph_memory_required(int max_tasks, int max_edges,
										 int n_threads)
{
	(void)n_threads;
	size_t sz = sizeof(struct FunComputeGraph_s);
	sz += (size_t)max_tasks * sizeof(TaskMeta);
	sz += (size_t)max_edges * sizeof(EdgeEntry);
	sz += (size_t)max_tasks * sizeof(int);
	return sz;
}

FunComputeGraph fun_compute_graph_init(void *memory, size_t memory_size,
									   int max_tasks, int max_edges,
									   int n_threads)
{
	size_t required =
		fun_compute_graph_memory_required(max_tasks, max_edges, n_threads);
	if (memory_size < required)
		return NULL;

	unsigned char *base = (unsigned char *)memory;
	struct FunComputeGraph_s *g = (struct FunComputeGraph_s *)memory;
	base += sizeof(struct FunComputeGraph_s);

	g->meta = (TaskMeta *)base;
	base += (size_t)max_tasks * sizeof(TaskMeta);

	g->edges = (EdgeEntry *)base;
	base += (size_t)max_edges * sizeof(EdgeEntry);

	g->ready_queue = (int *)base;

	g->n_tasks = 0;
	g->max_tasks = max_tasks;
	g->n_edges = 0;
	g->max_edges = max_edges;
	g->ready_head = 0;
	g->ready_tail = 0;
	g->ready_count = 0;
	g->pending_count = 0;
	g->workers_running = 0;
	g->n_threads = n_threads;
	g->thread_pool = NULL;
	g->ready_mutex = NULL;
	g->ready_condvar = NULL;
	g->done_mutex = NULL;
	g->done_condvar = NULL;

	for (int i = 0; i < max_tasks; i++) {
		g->meta[i].caller_task = NULL;
		g->meta[i].ndeps_total = 0;
		g->meta[i].ndeps_remaining = 0;
		g->meta[i].first_edge = -1;
		g->meta[i].ndependents = 0;
	}

	if (n_threads > 0) {
		if (fun_error_is_error(fun_mutex_create(&g->ready_mutex).error))
			return NULL;
		if (fun_error_is_error(fun_condvar_create(&g->ready_condvar).error))
			return NULL;
		if (fun_error_is_error(fun_mutex_create(&g->done_mutex).error))
			return NULL;
		if (fun_error_is_error(fun_condvar_create(&g->done_condvar).error))
			return NULL;
		ThreadPool tp = NULL;
		if (fun_error_is_error(
				fun_thread_pool_create((int32_t)n_threads, &tp).error) ||
			tp == NULL) {
			fun_condvar_destroy(g->done_condvar);
			fun_mutex_destroy(g->done_mutex);
			fun_condvar_destroy(g->ready_condvar);
			fun_mutex_destroy(g->ready_mutex);
			return NULL;
		}
		g->thread_pool = tp;
	}

	return g;
}

void fun_compute_graph_add_task(FunComputeGraph graph, FunComputeTask *task,
								FunComputeFn fn, void *ctx,
								FunComputeBindFn bind,
								FunComputeCtxDestroyFn destroy)
{
	int i = graph->n_tasks++;
	task->fn = fn;
	task->bind = bind;
	task->destroy = destroy;
	task->ctx = ctx;
	graph->meta[i].caller_task = task;
}

void fun_compute_task_depends_on(FunComputeGraph graph, FunComputeTask *task,
								 FunComputeTask *dep)
{
	int di = _find_task_index(graph, dep);
	int ti = _find_task_index(graph, task);
	int e = graph->n_edges++;
	graph->edges[e].dependent = ti;
	graph->edges[e].next = graph->meta[di].first_edge;
	graph->meta[di].first_edge = e;
	graph->meta[di].ndependents++;
	graph->meta[ti].ndeps_total++;
}

void fun_compute_graph_submit(FunComputeGraph graph, void *submit_ctx)
{
	for (int i = 0; i < graph->n_tasks; i++) {
		FunComputeTask *t = graph->meta[i].caller_task;
		if (t->bind)
			t->bind(t->ctx, submit_ctx);
		graph->meta[i].ndeps_remaining = graph->meta[i].ndeps_total;
	}

	graph->ready_head = 0;
	graph->ready_tail = 0;
	graph->ready_count = 0;
	graph->pending_count = graph->n_tasks;

	for (int i = 0; i < graph->n_tasks; i++) {
		if (graph->meta[i].ndeps_total == 0)
			_enqueue_ready(graph, i);
	}

	if (graph->n_threads > 0) {
		WorkerCtx wctx = { graph };
		WorkItem item = { .data = &wctx,
						  .data_size = sizeof(WorkerCtx),
						  .work_fn = _worker_loop };
		fun_mutex_lock(graph->done_mutex);
		graph->workers_running = 0;
		for (int i = 0; i < graph->n_threads; i++)
			if (!fun_error_is_error(
					fun_thread_pool_submit(graph->thread_pool, &item).error))
				graph->workers_running++;
		if (graph->workers_running == 0)
			fun_condvar_signal(graph->done_condvar);
		fun_mutex_unlock(graph->done_mutex);
	}
}

void fun_compute_graph_wait(FunComputeGraph graph)
{
	if (graph->n_threads == 0) {
		while (graph->ready_count > 0) {
			int idx = _dequeue_ready(graph);
			graph->meta[idx].caller_task->fn(graph->meta[idx].caller_task->ctx);
			_complete_task(graph, idx);
			graph->pending_count--;
		}
		return;
	}

	fun_mutex_lock(graph->done_mutex);
	while (graph->pending_count > 0 || graph->workers_running > 0)
		fun_condvar_wait(graph->done_condvar, graph->done_mutex);
	fun_mutex_unlock(graph->done_mutex);
}

void fun_compute_graph_destroy(FunComputeGraph graph)
{
	for (int i = 0; i < graph->n_tasks; i++) {
		FunComputeTask *t = graph->meta[i].caller_task;
		if (t && t->destroy)
			t->destroy(t->ctx);
	}
	if (graph->ready_condvar && graph->ready_mutex) {
		fun_mutex_lock(graph->ready_mutex);
		fun_condvar_broadcast(graph->ready_condvar);
		fun_mutex_unlock(graph->ready_mutex);
	}
	if (graph->thread_pool)
		fun_thread_pool_destroy(graph->thread_pool);
	if (graph->done_condvar)
		fun_condvar_destroy(graph->done_condvar);
	if (graph->done_mutex)
		fun_mutex_destroy(graph->done_mutex);
	if (graph->ready_condvar)
		fun_condvar_destroy(graph->ready_condvar);
	if (graph->ready_mutex)
		fun_mutex_destroy(graph->ready_mutex);
}
