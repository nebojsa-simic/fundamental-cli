#pragma once
#include "fundamental/file/file.h"
#include <stdint.h>
#include <windows.h>

// EMA time constant: 15-second window.
// Threshold: ops/sec above this → ring (database-like), below → mmap (web-like).
#define FILE_EMA_TAU 15.0f
#define FILE_IOPS_DB_THRESHOLD 50.0f

static inline uint64_t file_get_monotonic_ns(void)
{
	LARGE_INTEGER freq, counter;
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&counter);
	return (uint64_t)((double)counter.QuadPart / (double)freq.QuadPart * 1e9);
}

static inline FileMode file_adaptive_choose(FileAdaptiveState *state)
{
	if (!state || state->last_op_ns == 0)
		return FILE_MODE_MMAP;
	return (state->iops_ema >= FILE_IOPS_DB_THRESHOLD) ? FILE_MODE_RING_BASED :
														 FILE_MODE_MMAP;
}

// alpha = dt / (dt + tau): Pade approximant of 1-exp(-dt/tau), no math.h needed.
static inline void file_adaptive_update(FileAdaptiveState *state,
										uint64_t bytes)
{
	if (!state)
		return;
	uint64_t now_ns = file_get_monotonic_ns();
	if (state->last_op_ns == 0) {
		state->iops_ema = 0.0f;
		state->bytes_ema = 0.0f;
		state->last_op_ns = now_ns;
		return;
	}
	float dt = (float)(now_ns - state->last_op_ns) * 1e-9f;
	if (dt < 1e-9f)
		dt = 1e-9f;
	float alpha = dt / (dt + FILE_EMA_TAU);
	state->iops_ema = alpha * (1.0f / dt) + (1.0f - alpha) * state->iops_ema;
	state->bytes_ema =
		alpha * ((float)bytes / dt) + (1.0f - alpha) * state->bytes_ema;
	state->last_op_ns = now_ns;
}
