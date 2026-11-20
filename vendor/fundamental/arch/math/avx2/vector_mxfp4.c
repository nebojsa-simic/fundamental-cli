#include "fundamental/math/math.h"
#include <immintrin.h>

/* MXFP4 E2M1 kvalue table multiplied by two: the nibble lookup is done with
 * vpshufb over this table, so the scale byte (a power of two) is applied one
 * exponent lower: scale_f = bit_cast<float>((scale - 1) << 23). A scale byte
 * of zero decodes to 0.0f and skips the block entirely; a scale byte of 1
 * decodes to 2^-127, which the shifted exponent cannot represent (it would
 * be 0.0f), so it is handled with an explicit subnormal bit pattern. */
static const int8_t mxfp4_kvalues_x2[16] = { 0, 1,	2,	3,	4,	6,	8,	12,
											 0, -1, -2, -3, -4, -6, -8, -12 };

static inline float __attribute__((always_inline)) _mm256_hsum_ps(__m256 v)
{
	__m128 hi = _mm256_extractf128_ps(v, 1);
	__m128 lo = _mm256_castps256_ps128(v);
	__m128 sum = _mm_add_ps(lo, hi);
	sum = _mm_hadd_ps(sum, sum);
	sum = _mm_hadd_ps(sum, sum);
	return _mm_cvtss_f32(sum);
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
	__m128i tab = _mm_loadu_si128((const __m128i *)mxfp4_kvalues_x2);
	__m128i mask04 = _mm_set1_epi8(0x0F);
	__m256 sv_scale1 = _mm256_castsi256_ps(_mm256_set1_epi32(0x00400000u));

	for (size_t r = 0; r < rows; r++) {
		const uint8_t *bp = w + r * stride;
		__m256 acc = _mm256_setzero_ps();
		for (size_t b = 0; b < blocks; b++, bp += FUN_MATH_MXFP4_BLOCK_BYTES) {
			uint8_t scale = bp[0];
			if (scale == 0)
				continue;
			__m256 sv;
			if (scale == 1)
				sv = sv_scale1; /* 2^-128, pairs with the x2 table */
			else
				sv = _mm256_castsi256_ps(
					_mm256_set1_epi32(((uint32_t)(scale - 1)) << 23));
			__m128i n = _mm_loadu_si128((const __m128i *)(bp + 1));
			__m128i lo_i = _mm_shuffle_epi8(tab, _mm_and_si128(n, mask04));
			__m128i hi_i = _mm_shuffle_epi8(
				tab, _mm_and_si128(_mm_srli_epi16(n, 4), mask04));
			/* Accumulate block dot-product unscaled first,
			 * then apply the scale (matching gguf_dequant's
			 * rounding order: weight = k * sf is exact, FMA
			 * needs only one rounding). */
			const float *xv = x + b * FUN_MATH_MXFP4_BLOCK_ELEMS;
			__m256 k0 = _mm256_cvtepi32_ps(_mm256_cvtepi8_epi32(lo_i));
			__m256 k1 = _mm256_cvtepi32_ps(
				_mm256_cvtepi8_epi32(_mm_srli_si128(lo_i, 8)));
			__m256 k2 = _mm256_cvtepi32_ps(_mm256_cvtepi8_epi32(hi_i));
			__m256 k3 = _mm256_cvtepi32_ps(
				_mm256_cvtepi8_epi32(_mm_srli_si128(hi_i, 8)));
			__m256 bs = _mm256_setzero_ps();
			bs = _mm256_fmadd_ps(k0, _mm256_loadu_ps(xv), bs);
			bs = _mm256_fmadd_ps(k1, _mm256_loadu_ps(xv + 8), bs);
			bs = _mm256_fmadd_ps(k2, _mm256_loadu_ps(xv + 16), bs);
			bs = _mm256_fmadd_ps(k3, _mm256_loadu_ps(xv + 24), bs);
			acc = _mm256_fmadd_ps(bs, sv, acc);
		}
		float s = _mm256_hsum_ps(acc);
		out[r] = bias ? s + bias[r] : s;
	}
}
