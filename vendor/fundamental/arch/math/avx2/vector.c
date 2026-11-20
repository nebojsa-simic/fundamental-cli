#include "fundamental/math/math.h"
#include <immintrin.h>

#define L2E 1.4426950408889634f
#define LN2 0.6931471805599453f
#define PI 3.14159265358979323846f
#define TWOPI 6.28318530717958647692f
#define HALFPI 1.57079632679489661923f

static inline __m256 __attribute__((always_inline)) _mm256_exp_ps(__m256 x)
{
	__m256 l2e = _mm256_set1_ps(L2E);
	__m256 half = _mm256_set1_ps(0.5f);
	__m256 kf = _mm256_add_ps(_mm256_mul_ps(x, l2e), half);
	__m256i k = _mm256_cvtps_epi32(kf);

	__m256 ln2 = _mm256_set1_ps(LN2);
	__m256 r = _mm256_fnmadd_ps(_mm256_cvtepi32_ps(k), ln2, x);

	__m256 c0 = _mm256_set1_ps(1.0f);
	__m256 c1 = _mm256_set1_ps(0.5f);
	__m256 c2 = _mm256_set1_ps(0.10714285714285714f);
	__m256 c3 = _mm256_set1_ps(0.011904761904761904f);
	__m256 c4 = _mm256_set1_ps(0.0005952380952380952f);

	__m256 r2 = _mm256_mul_ps(r, r);

	__m256 num = _mm256_fmadd_ps(c1, r, c0);
	num = _mm256_fmadd_ps(c2, r2, num);
	num = _mm256_fmadd_ps(c3, _mm256_mul_ps(r, r2), num);
	num = _mm256_fmadd_ps(c4, _mm256_mul_ps(r2, r2), num);

	__m256 den = _mm256_fnmadd_ps(c1, r, c0);
	den = _mm256_fmadd_ps(c2, r2, den);
	den = _mm256_fnmadd_ps(c3, _mm256_mul_ps(r, r2), den);
	den = _mm256_fmadd_ps(c4, _mm256_mul_ps(r2, r2), den);

	__m256 er = _mm256_div_ps(num, den);

	__m256i exp_bits =
		_mm256_slli_epi32(_mm256_add_epi32(k, _mm256_set1_epi32(127)), 23);
	__m256 pow2 = _mm256_castsi256_ps(exp_bits);

	return _mm256_mul_ps(er, pow2);
}

static inline __m256 __attribute__((always_inline)) _mm256_sigmoid_ps(__m256 x)
{
	__m256 one = _mm256_set1_ps(1.0f);
	__m256 zero = _mm256_set1_ps(0.0f);
	__m256 pos_hi = _mm256_cmp_ps(x, _mm256_set1_ps(20.0f), _CMP_GE_OQ);
	__m256 neg_hi = _mm256_cmp_ps(x, _mm256_set1_ps(-20.0f), _CMP_LE_OQ);

	__m256 nx = _mm256_sub_ps(zero, x);
	__m256 ex_neg = _mm256_div_ps(one, _mm256_add_ps(one, _mm256_exp_ps(nx)));

	__m256 result = ex_neg;

	result = _mm256_blendv_ps(result, one, pos_hi);
	result = _mm256_blendv_ps(result, zero, neg_hi);

	return result;
}

static inline __m256 __attribute__((always_inline)) _mm256_silu_ps(__m256 x)
{
	return _mm256_mul_ps(x, _mm256_sigmoid_ps(x));
}

static inline float __attribute__((always_inline)) _mm256_hsum_ps(__m256 v)
{
	__m128 hi = _mm256_extractf128_ps(v, 1);
	__m128 lo = _mm256_castps256_ps128(v);
	__m128 sum = _mm_add_ps(lo, hi);
	sum = _mm_hadd_ps(sum, sum);
	sum = _mm_hadd_ps(sum, sum);
	return _mm_cvtss_f32(sum);
}

__attribute__((weak)) void fun_math_silu_f32(const float *x, float *out,
											 size_t n)
{
	size_t i = 0;
	for (; i + 8 <= n; i += 8) {
		__m256 v = _mm256_loadu_ps(x + i);
		_mm256_storeu_ps(out + i, _mm256_silu_ps(v));
	}
	for (; i < n; i++)
		out[i] = fun_math_silu(x[i]);
}

__attribute__((weak)) void fun_math_swiglu_f32_cpu(const float *gate,
												   const float *up, float *out,
												   size_t n)
{
	size_t i = 0;
	for (; i + 8 <= n; i += 8) {
		__m256 g = _mm256_loadu_ps(gate + i);
		__m256 u = _mm256_loadu_ps(up + i);
		__m256 sg = _mm256_silu_ps(g);
		_mm256_storeu_ps(out + i, _mm256_mul_ps(sg, u));
	}
	for (; i < n; i++) {
		float s = fun_math_sigmoid(gate[i]);
		out[i] = gate[i] * s * up[i];
	}
}

__attribute__((weak)) void fun_math_rms_norm_f32_cpu(const float *x,
													 const float *weight,
													 float *out, size_t n,
													 float eps)
{
	__m256 ss = _mm256_setzero_ps();
	size_t i = 0;
	for (; i + 8 <= n; i += 8) {
		__m256 v = _mm256_loadu_ps(x + i);
		ss = _mm256_fmadd_ps(v, v, ss);
	}
	float ss_val = _mm256_hsum_ps(ss);
	for (; i < n; i++)
		ss_val += x[i] * x[i];

	float scale = 1.0f / __builtin_sqrtf(ss_val / (float)n + eps);
	__m256 s256 = _mm256_set1_ps(scale);
	for (i = 0; i + 8 <= n; i += 8) {
		__m256 v = _mm256_loadu_ps(x + i);
		__m256 w = _mm256_loadu_ps(weight + i);
		_mm256_storeu_ps(out + i, _mm256_mul_ps(_mm256_mul_ps(v, s256), w));
	}
	for (; i < n; i++)
		out[i] = x[i] * scale * weight[i];
}

__attribute__((weak)) void fun_math_softmax_f32_cpu(float *x, size_t n)
{
	if (n == 0)
		return;

	size_t i;
	float m = x[0];
	for (i = 1; i < n; i++)
		if (x[i] > m)
			m = x[i];

	float sum = 0.0f;
	__m256 m256 = _mm256_set1_ps(m);
	__m256 lo = _mm256_set1_ps(-80.0f);
	for (i = 0; i + 8 <= n; i += 8) {
		__m256 v = _mm256_loadu_ps(x + i);
		v = _mm256_sub_ps(v, m256);
		v = _mm256_max_ps(v, lo);
		__m256 e = _mm256_exp_ps(v);
		_mm256_storeu_ps(x + i, e);
		sum += _mm256_hsum_ps(e);
	}
	for (; i < n; i++) {
		float e = fun_math_exp(x[i] - m);
		if (e != e)
			e = 0.0f;
		x[i] = e;
		sum += e;
	}

	for (i = 0; i + 8 <= n; i += 8) {
		__m256 v = _mm256_loadu_ps(x + i);
		_mm256_storeu_ps(x + i, _mm256_div_ps(v, _mm256_set1_ps(sum)));
	}
	for (; i < n; i++)
		x[i] /= sum;
}

static inline __m256 __attribute__((always_inline)) _mm256_log_ps(__m256 x)
{
	__m256i bits = _mm256_castps_si256(x);

	__m256i exp_field =
		_mm256_and_si256(_mm256_srli_epi32(bits, 23), _mm256_set1_epi32(0xFF));
	__m256i exp_int = _mm256_sub_epi32(exp_field, _mm256_set1_epi32(127));
	__m256i mant_bits =
		_mm256_or_si256(_mm256_and_si256(bits, _mm256_set1_epi32(0x7FFFFF)),
						_mm256_set1_epi32(0x3F800000));
	__m256 m = _mm256_castsi256_ps(mant_bits);

	__m256 one = _mm256_set1_ps(1.0f);
	__m256 t = _mm256_div_ps(_mm256_sub_ps(m, one), _mm256_add_ps(m, one));
	__m256 t2 = _mm256_mul_ps(t, t);
	__m256 poly = _mm256_set1_ps(0.111111111f);
	poly = _mm256_fmadd_ps(t2, poly, _mm256_set1_ps(0.142857143f));
	poly = _mm256_fmadd_ps(t2, poly, _mm256_set1_ps(0.2f));
	poly = _mm256_fmadd_ps(t2, poly, _mm256_set1_ps(0.333333333f));
	__m256 bracket = _mm256_fmadd_ps(t2, poly, one);
	__m256 lg_m =
		_mm256_mul_ps(_mm256_mul_ps(t, _mm256_set1_ps(2.0f)), bracket);

	return _mm256_fmadd_ps(_mm256_cvtepi32_ps(exp_int), _mm256_set1_ps(LN2),
						   lg_m);
}

static inline __m256 __attribute__((always_inline)) _mm256_sin_ps(__m256 x)
{
	__m256 nf = _mm256_add_ps(_mm256_mul_ps(x, _mm256_set1_ps(1.0f / TWOPI)),
							  _mm256_set1_ps(0.5f));
	__m256i n = _mm256_cvttps_epi32(nf);
	__m256i less = _mm256_castps_si256(
		_mm256_cmp_ps(nf, _mm256_cvtepi32_ps(n), _CMP_LT_OQ));
	n = _mm256_add_epi32(n, _mm256_srai_epi32(less, 31));
	__m256 xr =
		_mm256_fnmadd_ps(_mm256_cvtepi32_ps(n), _mm256_set1_ps(TWOPI), x);

	__m256 negate = _mm256_cmp_ps(xr, _mm256_setzero_ps(), _CMP_LT_OQ);
	__m256 ax =
		_mm256_blendv_ps(xr, _mm256_sub_ps(_mm256_setzero_ps(), xr), negate);

	__m256 is_big = _mm256_cmp_ps(ax, _mm256_set1_ps(PI), _CMP_GT_OQ);
	ax = _mm256_blendv_ps(ax, _mm256_sub_ps(_mm256_set1_ps(TWOPI), ax), is_big);
	__m256 is_hi = _mm256_cmp_ps(ax, _mm256_set1_ps(HALFPI), _CMP_GT_OQ);
	ax = _mm256_blendv_ps(ax, _mm256_sub_ps(_mm256_set1_ps(PI), ax), is_hi);

	__m256 x2 = _mm256_mul_ps(ax, ax);
	__m256 x3 = _mm256_mul_ps(x2, ax);
	__m256 x5 = _mm256_mul_ps(x3, x2);
	__m256 x7 = _mm256_mul_ps(x5, x2);
	__m256 x9 = _mm256_mul_ps(x7, x2);
	__m256 result = ax;
	result = _mm256_fnmadd_ps(x3, _mm256_set1_ps(0.1666666667f), result);
	result = _mm256_fmadd_ps(x5, _mm256_set1_ps(0.008333333333f), result);
	result = _mm256_fnmadd_ps(x7, _mm256_set1_ps(0.0001984126984f), result);
	result = _mm256_fmadd_ps(x9, _mm256_set1_ps(0.000002755731922f), result);

	return _mm256_blendv_ps(result, _mm256_sub_ps(_mm256_setzero_ps(), result),
							negate);
}

__attribute__((weak)) void
fun_math_matrix_vector_f32_cpu(const float *w, const float *x,
							   const float *bias, float *out, size_t rows,
							   size_t cols)
{
	for (size_t r = 0; r < rows; r++) {
		const float *wr = w + r * cols;
		__m256 sum = _mm256_setzero_ps();
		size_t c = 0;
		for (; c + 8 <= cols; c += 8)
			sum = _mm256_fmadd_ps(_mm256_loadu_ps(wr + c),
								  _mm256_loadu_ps(x + c), sum);
		float s = _mm256_hsum_ps(sum);
		for (; c < cols; c++)
			s += wr[c] * x[c];
		out[r] = (bias ? bias[r] : 0.0f) + s;
	}
}

__attribute__((weak)) float fun_math_dot_f32_cpu(const float *a, const float *b,
												 size_t n)
{
	__m256 sum = _mm256_setzero_ps();
	size_t i = 0;
	for (; i + 8 <= n; i += 8)
		sum = _mm256_fmadd_ps(_mm256_loadu_ps(a + i), _mm256_loadu_ps(b + i),
							  sum);
	float s = _mm256_hsum_ps(sum);
	for (; i < n; i++)
		s += a[i] * b[i];
	return s;
}

__attribute__((weak)) void fun_math_exp_f32(const float *x, float *out,
											size_t n)
{
	size_t i = 0;
	__m256 lo = _mm256_set1_ps(-87.3365f);
	__m256 hi = _mm256_set1_ps(88.7228f);
	__m256 overflow_cmp = _mm256_set1_ps(88.7228f);
	__m256 underflow_cmp = _mm256_set1_ps(-87.3365f);
	__m256 pinf = _mm256_castsi256_ps(_mm256_set1_epi32(0x7F800000));
	__m256 pnan = _mm256_castsi256_ps(_mm256_set1_epi32(0x7FC00000));
	for (; i + 8 <= n; i += 8) {
		__m256 v = _mm256_loadu_ps(x + i);
		__m256 overflow = _mm256_cmp_ps(v, overflow_cmp, _CMP_GE_OQ);
		__m256 underflow = _mm256_cmp_ps(v, underflow_cmp, _CMP_LE_OQ);
		__m256 is_nan = _mm256_cmp_ps(v, v, _CMP_UNORD_Q);
		__m256 vc = _mm256_min_ps(_mm256_max_ps(v, lo), hi);
		__m256 e = _mm256_exp_ps(vc);
		e = _mm256_blendv_ps(e, pinf, overflow);
		e = _mm256_blendv_ps(e, _mm256_setzero_ps(), underflow);
		e = _mm256_blendv_ps(e, pnan, is_nan);
		_mm256_storeu_ps(out + i, e);
	}
	for (; i < n; i++)
		out[i] = fun_math_exp(x[i]);
}

__attribute__((weak)) void fun_math_log_f32(const float *x, float *out,
											size_t n)
{
	size_t i = 0;
	__m256 pneginf = _mm256_castsi256_ps(_mm256_set1_epi32(0xFF800000));
	__m256 pinf = _mm256_castsi256_ps(_mm256_set1_epi32(0x7F800000));
	__m256 pnan = _mm256_castsi256_ps(_mm256_set1_epi32(0x7FC00000));
	for (; i + 8 <= n; i += 8) {
		__m256 v = _mm256_loadu_ps(x + i);
		__m256 e = _mm256_log_ps(v);
		__m256i bits = _mm256_castps_si256(v);
		__m256 is_zero = _mm256_castsi256_ps(_mm256_cmpeq_epi32(
			_mm256_slli_epi32(bits, 1), _mm256_setzero_si256()));
		__m256 is_neg = _mm256_castsi256_ps(
			_mm256_cmpgt_epi32(_mm256_setzero_si256(), bits));
		__m256 is_pos_inf = _mm256_castsi256_ps(_mm256_cmpeq_epi32(
			_mm256_and_si256(bits, _mm256_set1_epi32(0x7FFFFFFF)),
			_mm256_set1_epi32(0x7F800000)));
		__m256 is_nan = _mm256_cmp_ps(v, v, _CMP_UNORD_Q);
		e = _mm256_blendv_ps(e, pneginf, is_zero);
		e = _mm256_blendv_ps(e, pnan, is_neg);
		e = _mm256_blendv_ps(e, pinf, is_pos_inf);
		e = _mm256_blendv_ps(e, pnan, is_nan);
		_mm256_storeu_ps(out + i, e);
	}
	for (; i < n; i++)
		out[i] = fun_math_log(x[i]);
}

__attribute__((weak)) void fun_math_sin_f32(const float *x, float *out,
											size_t n)
{
	size_t i = 0;
	for (; i + 8 <= n; i += 8)
		_mm256_storeu_ps(out + i, _mm256_sin_ps(_mm256_loadu_ps(x + i)));
	for (; i < n; i++)
		out[i] = fun_math_sin(x[i]);
}

__attribute__((weak)) void fun_math_cos_f32(const float *x, float *out,
											size_t n)
{
	size_t i = 0;
	for (; i + 8 <= n; i += 8) {
		__m256 v = _mm256_loadu_ps(x + i);
		v = _mm256_add_ps(v, _mm256_set1_ps(HALFPI));
		_mm256_storeu_ps(out + i, _mm256_sin_ps(v));
	}
	for (; i < n; i++)
		out[i] = fun_math_cos(x[i]);
}

__attribute__((weak)) void
fun_math_rotary_f32_cpu(const float *x, const float *cosv, const float *sinv,
						float *out, size_t n_heads, size_t half)
{
	for (size_t h = 0; h < n_heads; h++) {
		const float *xh = x + h * 2 * half;
		float *oh = out + h * 2 * half;
		size_t j = 0;
		for (; j + 8 <= half; j += 8) {
			__m256 a = _mm256_loadu_ps(xh + j);
			__m256 b = _mm256_loadu_ps(xh + j + half);
			__m256 c = _mm256_loadu_ps(cosv + j);
			__m256 s = _mm256_loadu_ps(sinv + j);
			_mm256_storeu_ps(oh + j,
							 _mm256_fmsub_ps(a, c, _mm256_mul_ps(b, s)));
			_mm256_storeu_ps(oh + j + half,
							 _mm256_fmadd_ps(a, s, _mm256_mul_ps(b, c)));
		}
		for (; j < half; j++) {
			float a = xh[j];
			float b = xh[j + half];
			oh[j] = a * cosv[j] - b * sinv[j];
			oh[j + half] = a * sinv[j] + b * cosv[j];
		}
	}
}

__attribute__((weak)) void
fun_math_rows_dot_f32_cpu(const float *q, const float *x, float *out,
						  size_t n_rows, size_t row_len, size_t row_stride,
						  float scale)
{
	__m256 s256 = _mm256_set1_ps(scale);
	for (size_t t = 0; t < n_rows; t++) {
		const float *row = x + t * row_stride;
		__m256 sum = _mm256_setzero_ps();
		size_t d = 0;
		for (; d + 8 <= row_len; d += 8)
			sum = _mm256_fmadd_ps(_mm256_loadu_ps(q + d),
								  _mm256_loadu_ps(row + d), sum);
		float s = _mm256_hsum_ps(sum);
		for (; d < row_len; d++)
			s += q[d] * row[d];
		out[t] = s * scale;
	}
}

__attribute__((weak)) void
fun_math_weighted_sum_f32_cpu(const float *wgt, const float *x, float *out,
							  size_t n_rows, size_t row_len, size_t row_stride)
{
	size_t d = 0;
	for (; d + 8 <= row_len; d += 8)
		_mm256_storeu_ps(out + d, _mm256_setzero_ps());
	for (; d < row_len; d++)
		out[d] = 0.0f;

	for (size_t t = 0; t < n_rows; t++) {
		__m256 w = _mm256_set1_ps(wgt[t]);
		const float *xr = x + t * row_stride;
		d = 0;
		for (; d + 8 <= row_len; d += 8) {
			__m256 acc = _mm256_loadu_ps(out + d);
			acc = _mm256_fmadd_ps(w, _mm256_loadu_ps(xr + d), acc);
			_mm256_storeu_ps(out + d, acc);
		}
		for (; d < row_len; d++)
			out[d] += wgt[t] * xr[d];
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

/* Weak so the AVX-512 implementation in avx512/vector.c takes precedence when
 * both objects are linked; the AVX2 build links this object alone and uses
 * this version. */
__attribute__((weak)) void
fun_math_q8_matrix_vector_f32_cpu(const uint8_t *w, const float *x, float *out,
								  size_t rows, size_t cols)
{
	const size_t blocks = cols / FUN_MATH_Q8_BLOCK_ELEMS;
	for (size_t r = 0; r < rows; r++) {
		const uint8_t *row_src = w + r * blocks * FUN_MATH_Q8_BLOCK_BYTES;
		__m256 acc = _mm256_setzero_ps();
		for (size_t b = 0; b < blocks; b++) {
			const uint8_t *bp = row_src + b * FUN_MATH_Q8_BLOCK_BYTES;
			float d =
				fun_math_fp16_to_f32((uint16_t)bp[0] | ((uint16_t)bp[1] << 8));
			const int8_t *q = (const int8_t *)(bp + 2);
			const float *xv = x + b * FUN_MATH_Q8_BLOCK_ELEMS;
			__m256 block_sum = _mm256_setzero_ps();
			for (size_t j = 0; j + 8 <= FUN_MATH_Q8_BLOCK_ELEMS; j += 8) {
				__m256i qi = _mm256_cvtepi8_epi32(
					_mm_loadl_epi64((const __m128i *)(q + j)));
				__m256 xvj = _mm256_loadu_ps(xv + j);
				block_sum =
					_mm256_fmadd_ps(_mm256_cvtepi32_ps(qi), xvj, block_sum);
			}
			acc = _mm256_fmadd_ps(_mm256_set1_ps(d), block_sum, acc);
		}
		out[r] = _mm256_hsum_ps(acc);
	}
}
