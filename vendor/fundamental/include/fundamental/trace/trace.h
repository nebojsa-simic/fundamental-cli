#ifndef LIBRARY_TRACE_H
#define LIBRARY_TRACE_H

#include <stddef.h>
#include <stdint.h>

typedef uint32_t FunSpanId;

typedef void (*FunTraceReportFn)(const char *line);

size_t fun_trace_memory_required(int max_spans);

void fun_trace_init(void *memory, size_t memory_size, int max_spans);

void fun_trace_destroy(void);

FunSpanId fun_trace_span_register(const char *name);

void fun_trace_span_begin(FunSpanId id);

void fun_trace_span_end(FunSpanId id);

void fun_trace_span_attribute_i64(FunSpanId id, const char *key, int64_t value);

void fun_trace_span_attribute_f64(FunSpanId id, const char *key, double value);

void fun_trace_span_attribute_str(FunSpanId id, const char *key,
								  const char *value);

void fun_trace_span_add_event(FunSpanId id, const char *event_name);

void fun_trace_counter_add(FunSpanId id, int64_t delta);

void fun_trace_report(FunTraceReportFn output_fn);

#endif
