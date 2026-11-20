#include "fundamental/math/math.h"
#include <stdint.h>

/* Portable scalar MXFP4 reference. Matches the AVX2/AVX512 kernels exactly:
 * the E2M1 kvalue table is pre-multiplied by two and the scale byte (a power
 * of two) is applied one exponent lower, so a scale byte of zero decodes to
 * 0.0f (skipped) and a scale byte of 255 never overflows to inf. */
static const float mxfp4_kvalues_x2[16] = { 0.0f,  1.0f,  2.0f,	 3.0f,
											4.0f,  6.0f,  8.0f,	 12.0f,
											-0.0f, -1.0f, -2.0f, -3.0f,
											-4.0f, -6.0f, -8.0f, -12.0f };

static float mxfp4_scale_f(uint8_t scale)
{
	if (scale == 1)
		return *(float *)&(uint32_t){ 0x00400000u };
	return *(float *)&(uint32_t){ (uint32_t)(scale - 1) << 23 };
}

void fun_math_matrix_vector_mxfp4_f32_cpu(const uint8_t *w, const float *x,
										  const float *bias, float *out,
										  size_t rows, size_t cols)
{
	fun_math_matrix_vector_mxfp4_f32_strided_cpu(w, x, bias, out, rows, cols,
												 0);
}

void fun_math_matrix_vector_mxfp4_f32_strided_cpu(const uint8_t *w,
												  const float *x,
												  const float *bias, float *out,
												  size_t rows, size_t cols,
												  size_t row_stride_bytes)
{
	const size_t blocks = cols / FUN_MATH_MXFP4_BLOCK_ELEMS;
	const size_t row_bytes = blocks * FUN_MATH_MXFP4_BLOCK_BYTES;
	const size_t stride = row_stride_bytes > 0 ? row_stride_bytes : row_bytes;

	for (size_t r = 0; r < rows; r++) {
		const uint8_t *bp = w + r * stride;
		float s = 0.0f;
		for (size_t b = 0; b < blocks; b++, bp += FUN_MATH_MXFP4_BLOCK_BYTES) {
			uint8_t scale = bp[0];
			if (scale == 0)
				continue;
			float sf = mxfp4_scale_f(scale);
			const float *xv = x + b * FUN_MATH_MXFP4_BLOCK_ELEMS;
			for (size_t j = 0; j < 16; j++) {
				uint8_t byte = bp[1 + j];
				s += mxfp4_kvalues_x2[byte & 0x0F] * sf * xv[j];
				s += mxfp4_kvalues_x2[byte >> 4] * sf * xv[j + 16];
			}
		}
		out[r] = bias ? s + bias[r] : s;
	}
}
