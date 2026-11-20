#ifndef LIBRARY_COMPUTE_H
#define LIBRARY_COMPUTE_H

#include <stdbool.h>
#include <stddef.h>

typedef void (*FunComputeFn)(void *ctx);
typedef void (*FunComputeBindFn)(void *task_ctx, void *submit_ctx);
typedef void (*FunComputeCtxDestroyFn)(void *ctx);

typedef struct FunComputeTask FunComputeTask;
typedef struct FunComputeGraph_s *FunComputeGraph;

struct FunComputeTask {
	FunComputeFn fn;
	FunComputeBindFn bind;
	FunComputeCtxDestroyFn destroy;
	void *ctx;
};

size_t fun_compute_graph_memory_required(int max_tasks, int max_edges,
										 int n_threads);

FunComputeGraph fun_compute_graph_init(void *memory, size_t memory_size,
									   int max_tasks, int max_edges,
									   int n_threads);

void fun_compute_graph_add_task(FunComputeGraph graph, FunComputeTask *task,
								FunComputeFn fn, void *ctx,
								FunComputeBindFn bind,
								FunComputeCtxDestroyFn destroy);

void fun_compute_task_depends_on(FunComputeGraph graph, FunComputeTask *task,
								 FunComputeTask *dep);

void fun_compute_graph_submit(FunComputeGraph graph, void *submit_ctx);

void fun_compute_graph_wait(FunComputeGraph graph);

void fun_compute_graph_destroy(FunComputeGraph graph);

#endif
