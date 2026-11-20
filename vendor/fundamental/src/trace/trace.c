#include "fundamental/trace/trace.h"
#include "fundamental/arch/tls.h"
#include "fundamental/memory/memory.h"
#include "fundamental/string/string.h"
#include "fundamental/timing/timing.h"

#define MAX_ATTRS 8

typedef struct {
	const char *key;
	int type;
	int64_t i64_val;
	double f64_val;
	const char *str_val;
} TraceAttribute;

typedef struct {
	const char *name;
	uint64_t total_ns;
	uint64_t call_count;
	int64_t counter;
	int n_events;
	const char *event_names[16];
	TraceAttribute attrs[MAX_ATTRS];
	int n_attrs;
} SpanSlot;

typedef struct {
	SpanSlot *slots;
	int max_spans;
	int n_spans;
} FunTracerInternal;

static FunTracerInternal *g_tracer = NULL;

size_t fun_trace_memory_required(int max_spans)
{
	return sizeof(FunTracerInternal) + (size_t)max_spans * sizeof(SpanSlot);
}

void fun_trace_init(void *memory, size_t memory_size, int max_spans)
{
	size_t required = fun_trace_memory_required(max_spans);
	if (memory_size < required)
		return;
	unsigned char *base = (unsigned char *)memory;
	FunTracerInternal *t = (FunTracerInternal *)memory;
	base += sizeof(FunTracerInternal);
	t->slots = (SpanSlot *)base;
	t->max_spans = max_spans;
	t->n_spans = 0;
	for (int i = 0; i < max_spans; i++) {
		t->slots[i].name = NULL;
		t->slots[i].total_ns = 0;
		t->slots[i].call_count = 0;
		t->slots[i].counter = 0;
		t->slots[i].n_events = 0;
		t->slots[i].n_attrs = 0;
	}
	g_tracer = t;
}

void fun_trace_destroy(void)
{
	g_tracer = NULL;
}

FunSpanId fun_trace_span_register(const char *name)
{
	if (!g_tracer)
		return 0;
	for (int i = 0; i < g_tracer->n_spans; i++)
		if (fun_string_compare(g_tracer->slots[i].name, name) == 0)
			return (FunSpanId)(i + 1);
	int id = g_tracer->n_spans++;
	g_tracer->slots[id].name = name;
	return (FunSpanId)(id + 1);
}

void fun_trace_span_begin(FunSpanId id)
{
	if (!g_tracer)
		return;
	fun_tls_u64_set(fun_timing_now_ns());
	(void)id;
}

void fun_trace_span_end(FunSpanId id)
{
	if (!g_tracer || id == 0 || (int)id > g_tracer->n_spans)
		return;
	uint64_t dt = fun_timing_now_ns() - fun_tls_u64_get();
	int idx = (int)id - 1;
	__atomic_fetch_add(&g_tracer->slots[idx].total_ns, dt, __ATOMIC_RELAXED);
	__atomic_fetch_add(&g_tracer->slots[idx].call_count, 1, __ATOMIC_RELAXED);
}

static void _trace_set_attribute(SpanSlot *slot, const char *key, int type,
								 int64_t i64_val, double f64_val,
								 const char *str_val)
{
	for (int i = 0; i < slot->n_attrs; i++) {
		if (fun_string_compare(slot->attrs[i].key, key) == 0) {
			slot->attrs[i].type = type;
			slot->attrs[i].i64_val = i64_val;
			slot->attrs[i].f64_val = f64_val;
			slot->attrs[i].str_val = str_val;
			return;
		}
	}
	if (slot->n_attrs >= MAX_ATTRS)
		return;
	int a = slot->n_attrs++;
	slot->attrs[a].key = key;
	slot->attrs[a].type = type;
	slot->attrs[a].i64_val = i64_val;
	slot->attrs[a].f64_val = f64_val;
	slot->attrs[a].str_val = str_val;
}

void fun_trace_span_attribute_i64(FunSpanId id, const char *key, int64_t value)
{
	if (!g_tracer || id == 0 || (int)id > g_tracer->n_spans)
		return;
	_trace_set_attribute(&g_tracer->slots[(int)id - 1], key, 1, value, 0, NULL);
}

void fun_trace_span_attribute_f64(FunSpanId id, const char *key, double value)
{
	if (!g_tracer || id == 0 || (int)id > g_tracer->n_spans)
		return;
	_trace_set_attribute(&g_tracer->slots[(int)id - 1], key, 2, 0, value, NULL);
}

void fun_trace_span_attribute_str(FunSpanId id, const char *key,
								  const char *value)
{
	if (!g_tracer || id == 0 || (int)id > g_tracer->n_spans)
		return;
	_trace_set_attribute(&g_tracer->slots[(int)id - 1], key, 3, 0, 0.0, value);
}

void fun_trace_span_add_event(FunSpanId id, const char *event_name)
{
	if (!g_tracer || id == 0 || (int)id > g_tracer->n_spans)
		return;
	int idx = (int)id - 1;
	if (g_tracer->slots[idx].n_events >= 16)
		return;
	g_tracer->slots[idx].event_names[g_tracer->slots[idx].n_events++] =
		event_name;
}

void fun_trace_counter_add(FunSpanId id, int64_t delta)
{
	if (!g_tracer || id == 0 || (int)id > g_tracer->n_spans)
		return;
	int idx = (int)id - 1;
	__atomic_fetch_add(&g_tracer->slots[idx].counter, delta, __ATOMIC_RELAXED);
}

void fun_trace_report(FunTraceReportFn out)
{
	if (!g_tracer || g_tracer->n_spans == 0 || !out)
		return;

	uint64_t total_ns = g_tracer->slots[0].total_ns;
	uint64_t sum_ns = 0;
	char line[256];

	fun_string_copy("Span                 Total(ms)  Count   Avg(us)     %",
					line, 256);
	out(line);
	fun_string_copy("-------------------------------------------"
					"---------------",
					line, 256);
	out(line);

	for (int i = 0; i < g_tracer->n_spans; i++) {
		SpanSlot *s = &g_tracer->slots[i];
		double ms = (double)s->total_ns / 1e6;
		double avg_us = s->call_count > 0 ?
							(double)s->total_ns / (double)s->call_count / 1e3 :
							0.0;
		double pct =
			total_ns > 0 ? 100.0 * (double)s->total_ns / (double)total_ns : 0.0;

		StringTemplateParam params[] = {
			{ "name", { .stringValue = s->name ? s->name : "?" } },
			{ "ms", { .doubleValue = ms } },
			{ "count", { .intValue = (int64_t)s->call_count } },
			{ "avg", { .doubleValue = avg_us } },
			{ "pct", { .doubleValue = pct } },
		};

		fun_string_template("  ${name}  %{ms}  #{count}  %{avg}  %{pct}",
							params, 5, line, 256);
		out(line);

		if (i == 0) {
			fun_string_copy("-------------------------------------"
							"---------------------",
							line, 256);
			out(line);
			continue;
		}
		sum_ns += s->total_ns;
	}

	if (total_ns > 0 && total_ns > sum_ns) {
		uint64_t oh_ns = total_ns - sum_ns;
		double oh_ms = (double)oh_ns / 1e6;
		double oh_pct = 100.0 * (double)oh_ns / (double)total_ns;

		fun_string_copy("---------------------------------------------"
						"-------------",
						line, 256);
		out(line);

		StringTemplateParam oparams[] = {
			{ "ms", { .doubleValue = oh_ms } },
			{ "pct", { .doubleValue = oh_pct } },
		};
		fun_string_template("  overhead  %{ms}ms  %{pct}%", oparams, 2, line,
							256);
		out(line);
	}
}
