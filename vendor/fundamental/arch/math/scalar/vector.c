#include "fundamental/math/math.h"
#include <stdint.h>

/* Portable scalar implementations of the vectorized batch API. Provides a
 * lowest-common-denominator build target (SSE/scalar) independent of AVX. */

void fun_math_silu_f32(const float *x, float *out, size_t n)
{
	for (size_t i = 0; i < n; i++)
		out[i] = fun_math_silu(x[i]);
}

void fun_math_swiglu_f32_cpu(const float *gate, const float *up, float *out,
							 size_t n)
{
	for (size_t i = 0; i < n; i++)
		out[i] = gate[i] * fun_math_sigmoid(gate[i]) * up[i];
}

void fun_math_rms_norm_f32_cpu(const float *x, const float *weight, float *out,
							   size_t n, float eps)
{
	float ss = 0.0f;
	for (size_t i = 0; i < n; i++)
		ss += x[i] * x[i];
	float scale = 1.0f / fun_math_sqrt(ss / (float)n + eps);
	for (size_t i = 0; i < n; i++)
		out[i] = x[i] * scale * weight[i];
}

void fun_math_softmax_f32_cpu(float *x, size_t n)
{
	if (n == 0)
		return;
	float m = x[0];
	for (size_t i = 1; i < n; i++)
		if (x[i] > m)
			m = x[i];
	float sum = 0.0f;
	for (size_t i = 0; i < n; i++) {
		float e = fun_math_exp(x[i] - m);
		x[i] = e;
		sum += e;
	}
	for (size_t i = 0; i < n; i++)
		x[i] /= sum;
}

void fun_math_matrix_vector_f32_cpu(const float *w, const float *x,
									const float *bias, float *out, size_t rows,
									size_t cols)
{
	for (size_t r = 0; r < rows; r++) {
		const float *wr = w + r * cols;
		float s = 0.0f;
		for (size_t c = 0; c < cols; c++)
			s += wr[c] * x[c];
		out[r] = (bias ? bias[r] : 0.0f) + s;
	}
}

float fun_math_dot_f32_cpu(const float *a, const float *b, size_t n)
{
	float s = 0.0f;
	for (size_t i = 0; i < n; i++)
		s += a[i] * b[i];
	return s;
}

void fun_math_exp_f32(const float *x, float *out, size_t n)
{
	for (size_t i = 0; i < n; i++)
		out[i] = fun_math_exp(x[i]);
}

void fun_math_log_f32(const float *x, float *out, size_t n)
{
	for (size_t i = 0; i < n; i++)
		out[i] = fun_math_log(x[i]);
}

void fun_math_sin_f32(const float *x, float *out, size_t n)
{
	for (size_t i = 0; i < n; i++)
		out[i] = fun_math_sin(x[i]);
}

void fun_math_cos_f32(const float *x, float *out, size_t n)
{
	for (size_t i = 0; i < n; i++)
		out[i] = fun_math_cos(x[i]);
}

void fun_math_rotary_f32_cpu(const float *x, const float *cosv,
							 const float *sinv, float *out, size_t n_heads,
							 size_t half)
{
	for (size_t h = 0; h < n_heads; h++) {
		const float *xh = x + h * 2 * half;
		float *oh = out + h * 2 * half;
		for (size_t j = 0; j < half; j++) {
			float a = xh[j];
			float b = xh[j + half];
			oh[j] = a * cosv[j] - b * sinv[j];
			oh[j + half] = a * sinv[j] + b * cosv[j];
		}
	}
}

void fun_math_rows_dot_f32_cpu(const float *q, const float *x, float *out,
							   size_t n_rows, size_t row_len, size_t row_stride,
							   float scale)
{
	for (size_t t = 0; t < n_rows; t++) {
		const float *row = x + t * row_stride;
		float s = 0.0f;
		for (size_t d = 0; d < row_len; d++)
			s += q[d] * row[d];
		out[t] = s * scale;
	}
}

void fun_math_weighted_sum_f32_cpu(const float *wgt, const float *x, float *out,
								   size_t n_rows, size_t row_len,
								   size_t row_stride)
{
	for (size_t d = 0; d < row_len; d++) {
		float acc = 0.0f;
		for (size_t t = 0; t < n_rows; t++)
			acc += wgt[t] * x[t * row_stride + d];
		out[d] = acc;
	}
}

void fun_math_q8_dequant_row_f32_cpu(const uint8_t *src, float *out, size_t n)
{
	size_t blocks = n / FUN_MATH_Q8_BLOCK_ELEMS;
	for (size_t b = 0; b < blocks; b++) {
		float d = fun_math_fp16_to_f32(
			(uint16_t)src[b * FUN_MATH_Q8_BLOCK_BYTES] |
			((uint16_t)src[b * FUN_MATH_Q8_BLOCK_BYTES + 1] << 8));
		const int8_t *q =
			(const int8_t *)(src + b * FUN_MATH_Q8_BLOCK_BYTES + 2);
		for (size_t j = 0; j < FUN_MATH_Q8_BLOCK_ELEMS; j++)
			out[b * FUN_MATH_Q8_BLOCK_ELEMS + j] = (float)q[j] * d;
	}
}

void fun_math_q8_matrix_vector_f32_cpu(const uint8_t *w, const float *x,
									   float *out, size_t rows, size_t cols)
{
	const size_t blocks = cols / FUN_MATH_Q8_BLOCK_ELEMS;
	for (size_t r = 0; r < rows; r++) {
		const uint8_t *row_src = w + r * blocks * FUN_MATH_Q8_BLOCK_BYTES;
		float s = 0.0f;
		for (size_t b = 0; b < blocks; b++) {
			const uint8_t *bp = row_src + b * FUN_MATH_Q8_BLOCK_BYTES;
			float d =
				fun_math_fp16_to_f32((uint16_t)bp[0] | ((uint16_t)bp[1] << 8));
			const int8_t *q = (const int8_t *)(bp + 2);
			const float *xv = x + b * FUN_MATH_Q8_BLOCK_ELEMS;
			for (size_t j = 0; j < FUN_MATH_Q8_BLOCK_ELEMS; j++)
				s += (float)q[j] * d * xv[j];
		}
		out[r] = s;
	}
}
