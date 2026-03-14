#pragma once
#include "file/file.h"
#include <stdint.h>

#define FILE_EMA_TAU 15.0f
#define FILE_IOPS_DB_THRESHOLD 50.0f

#define FILE_SYS_clock_gettime 228
#define FILE_CLOCK_MONOTONIC 1

struct file_timespec {
	long tv_sec;
	long tv_nsec;
};

static inline long file_syscall2_clk(long n, long a1, long a2)
{
	long ret;
	__asm__ __volatile__("syscall"
						 : "=a"(ret)
						 : "a"(n), "D"(a1), "S"(a2)
						 : "rcx", "r11", "memory");
	return ret;
}

static inline uint64_t file_get_monotonic_ns(void)
{
	struct file_timespec ts;
	file_syscall2_clk(FILE_SYS_clock_gettime, FILE_CLOCK_MONOTONIC, (long)&ts);
	return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static inline FileMode file_adaptive_choose(FileAdaptiveState *state)
{
	if (!state || state->last_op_ns == 0)
		return FILE_MODE_MMAP;
	return (state->iops_ema >= FILE_IOPS_DB_THRESHOLD) ? FILE_MODE_RING_BASED :
														 FILE_MODE_MMAP;
}

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
