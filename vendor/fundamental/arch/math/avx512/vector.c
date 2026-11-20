#include "fundamental/math/math.h"
#include <immintrin.h>

#define L2E 1.4426950408889634f
#define LN2 0.6931471805599453f
#define PI 3.14159265358979323846f
#define TWOPI 6.28318530717958647692f
#define HALFPI 1.57079632679489661923f

/* MXFP4 E2M1 kvalue table multiplied by two: the nibble lookup is done with
 * vpshufb over this table, so the scale byte (a power of two) is applied one
 * exponent lower: scale_f = bit_cast<float>((scale - 1) << 23). A scale byte
 * of zero decodes to 0.0f and skips the block entirely; a scale byte of 1
 * decodes to 2^-127, which the shifted exponent cannot represent (it would
 * be 0.0f), so it is handled with an explicit subnormal bit pattern. */
static const int8_t mxfp4_kvalues_x2[16] = { 0, 1,	2,	3,	4,	6,	8,	12,
											 0, -1, -2, -3, -4, -6, -8, -12 };

/* Decode a 16-nibble MXFP4 block payload into two __m512 float kvalue vectors
 * (low nibbles -> elements 0..15, high nibbles -> elements 16..31): two
 * 128-bit vpshufb lookups, then 512-bit sign-extend and int->float. */
static inline void __attribute__((always_inline))
_mxfp4_dequant_block512(const uint8_t *nibbles, const __m128i tab,
						const __m128i mask04, __m512 *k0, __m512 *k1)
{
	__m128i n = _mm_loadu_si128((const __m128i *)nibbles);
	__m128i lo_i = _mm_shuffle_epi8(tab, _mm_and_si128(n, mask04));
	__m128i hi_i =
		_mm_shuffle_epi8(tab, _mm_and_si128(_mm_srli_epi16(n, 4), mask04));
	*k0 = _mm512_cvtepi32_ps(_mm512_cvtepi8_epi32(lo_i));
	*k1 = _mm512_cvtepi32_ps(_mm512_cvtepi8_epi32(hi_i));
}

/* Broadcast a block scale byte to 16 lanes. scale decodes to 2^(scale-128)
 * (one exponent below the E8M0 value because the kvalue table is doubled);
 * scale==1 needs the explicit subnormal 2^-127 bit pattern. */
static inline __m512 __attribute__((always_inline))
_mxfp4_scale_broadcast512(uint8_t scale, __m512 sv_scale1)
{
	if (scale == 1)
		return sv_scale1;
	return _mm512_castsi512_ps(
		_mm512_set1_epi32(((uint32_t)(scale - 1)) << 23));
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
	__m512 sv_scale1 = _mm512_castsi512_ps(_mm512_set1_epi32(0x00400000u));

	for (size_t r = 0; r < rows; r++) {
		const uint8_t *bp = w + r * stride;
		__m512 acc = _mm512_setzero_ps();

		/* 16-lane accumulator: lane i sums elements i and i+16 of each
		 * 32-element block, keeping 16 independent partial sums reduced once
		 * per row instead of once per block. */
		for (size_t b = 0; b < blocks; b++, bp += FUN_MATH_MXFP4_BLOCK_BYTES) {
			uint8_t scale = bp[0];
			if (scale == 0)
				continue;

			const float *xv = x + b * FUN_MATH_MXFP4_BLOCK_ELEMS;
			__m512 k0, k1;
			_mxfp4_dequant_block512(bp + 1, tab, mask04, &k0, &k1);

			__m512 bs = _mm512_mul_ps(k0, _mm512_loadu_ps(xv));
			bs = _mm512_fmadd_ps(k1, _mm512_loadu_ps(xv + 16), bs);
			acc = _mm512_fmadd_ps(
				bs, _mxfp4_scale_broadcast512(scale, sv_scale1), acc);
		}

		float s = _mm512_reduce_add_ps(acc);
		out[r] = bias ? s + bias[r] : s;
	}
}

/* Q8_0 matrix-vector product: out[r] = sum_d w[r][d] * x[d]. The weight matrix
 * is stored in 34-byte blocks (2-byte fp16 scale + 32 int8 values); each block
 * is dequantized on the fly by sign-extending the int8 values to f32 and
 * scaling the block dot product by the fp16 scale. Processes 16 elements per
 * 512-bit op (twice the AVX2 throughput), accelerating the attention and
 * output projections that dominate single-token decode. */
void fun_math_q8_matrix_vector_f32_cpu(const uint8_t *w, const float *x,
									   float *out, size_t rows, size_t cols)
{
	const size_t blocks = cols / FUN_MATH_Q8_BLOCK_ELEMS;
	for (size_t r = 0; r < rows; r++) {
		const uint8_t *row_src = w + r * blocks * FUN_MATH_Q8_BLOCK_BYTES;
		__m512 acc = _mm512_setzero_ps();
		for (size_t b = 0; b < blocks; b++) {
			const uint8_t *bp = row_src + b * FUN_MATH_Q8_BLOCK_BYTES;
			float d =
				fun_math_fp16_to_f32((uint16_t)bp[0] | ((uint16_t)bp[1] << 8));
			const int8_t *q = (const int8_t *)(bp + 2);
			const float *xv = x + b * FUN_MATH_Q8_BLOCK_ELEMS;

			__m512i q0 =
				_mm512_cvtepi8_epi32(_mm_loadu_si128((const __m128i *)q));
			__m512 bs =
				_mm512_mul_ps(_mm512_cvtepi32_ps(q0), _mm512_loadu_ps(xv));
			__m512i q1 = _mm512_cvtepi8_epi32(
				_mm_loadu_si128((const __m128i *)(q + 16)));
			bs = _mm512_fmadd_ps(_mm512_cvtepi32_ps(q1),
								 _mm512_loadu_ps(xv + 16), bs);
			acc = _mm512_fmadd_ps(_mm512_set1_ps(d), bs, acc);
		}
		out[r] = _mm512_reduce_add_ps(acc);
	}
}

/* ----------------------------------------------------------------------------
 * AVX-512 f32 elementwise and matvec kernels. These mirror the AVX2 versions
 * in arch/math/avx2/vector.c but process 16 lanes per op, roughly halving the
 * instruction count on AVX-512 hardware. They are strong symbols here; the
 * AVX2 counterparts are weak, so this object wins when both are linked (the
 * default and test builds), while -avx2/-sse builds link only the AVX2/scalar
 * copies.
 * ------------------------------------------------------------------------- */

static inline __m512 __attribute__((always_inline)) _mm512_exp_ps(__m512 x)
{
	__m512 l2e = _mm512_set1_ps(L2E);
	__m512 half = _mm512_set1_ps(0.5f);
	__m512 kf = _mm512_add_ps(_mm512_mul_ps(x, l2e), half);
	__m512i k = _mm512_cvtps_epi32(kf);

	__m512 ln2 = _mm512_set1_ps(LN2);
	__m512 r = _mm512_fnmadd_ps(_mm512_cvtepi32_ps(k), ln2, x);

	__m512 c0 = _mm512_set1_ps(1.0f);
	__m512 c1 = _mm512_set1_ps(0.5f);
	__m512 c2 = _mm512_set1_ps(0.10714285714285714f);
	__m512 c3 = _mm512_set1_ps(0.011904761904761904f);
	__m512 c4 = _mm512_set1_ps(0.0005952380952380952f);

	__m512 r2 = _mm512_mul_ps(r, r);

	__m512 num = _mm512_fmadd_ps(c1, r, c0);
	num = _mm512_fmadd_ps(c2, r2, num);
	num = _mm512_fmadd_ps(c3, _mm512_mul_ps(r, r2), num);
	num = _mm512_fmadd_ps(c4, _mm512_mul_ps(r2, r2), num);

	__m512 den = _mm512_fnmadd_ps(c1, r, c0);
	den = _mm512_fmadd_ps(c2, r2, den);
	den = _mm512_fnmadd_ps(c3, _mm512_mul_ps(r, r2), den);
	den = _mm512_fmadd_ps(c4, _mm512_mul_ps(r2, r2), den);

	__m512 er = _mm512_div_ps(num, den);

	__m512i exp_bits =
		_mm512_slli_epi32(_mm512_add_epi32(k, _mm512_set1_epi32(127)), 23);
	__m512 pow2 = _mm512_castsi512_ps(exp_bits);

	return _mm512_mul_ps(er, pow2);
}

static inline __m512 __attribute__((always_inline)) _mm512_sigmoid_ps(__m512 x)
{
	__m512 one = _mm512_set1_ps(1.0f);
	__m512 zero = _mm512_setzero_ps();
	__mmask16 pos_hi = _mm512_cmp_ps_mask(x, _mm512_set1_ps(20.0f), _CMP_GE_OQ);
	__mmask16 neg_hi =
		_mm512_cmp_ps_mask(x, _mm512_set1_ps(-20.0f), _CMP_LE_OQ);

	__m512 nx = _mm512_sub_ps(zero, x);
	__m512 ex_neg = _mm512_div_ps(one, _mm512_add_ps(one, _mm512_exp_ps(nx)));

	__m512 result = _mm512_mask_blend_ps(pos_hi, ex_neg, one);
	result = _mm512_mask_blend_ps(neg_hi, result, zero);

	return result;
}

static inline __m512 __attribute__((always_inline)) _mm512_silu_ps(__m512 x)
{
	return _mm512_mul_ps(x, _mm512_sigmoid_ps(x));
}

static inline __m512 __attribute__((always_inline)) _mm512_log_ps(__m512 x)
{
	__m512i bits = _mm512_castps_si512(x);

	__m512i exp_field =
		_mm512_and_si512(_mm512_srli_epi32(bits, 23), _mm512_set1_epi32(0xFF));
	__m512i exp_int = _mm512_sub_epi32(exp_field, _mm512_set1_epi32(127));
	__m512i mant_bits =
		_mm512_or_si512(_mm512_and_si512(bits, _mm512_set1_epi32(0x7FFFFF)),
						_mm512_set1_epi32(0x3F800000));
	__m512 m = _mm512_castsi512_ps(mant_bits);

	__m512 one = _mm512_set1_ps(1.0f);
	__m512 t = _mm512_div_ps(_mm512_sub_ps(m, one), _mm512_add_ps(m, one));
	__m512 t2 = _mm512_mul_ps(t, t);
	__m512 poly = _mm512_set1_ps(0.111111111f);
	poly = _mm512_fmadd_ps(t2, poly, _mm512_set1_ps(0.142857143f));
	poly = _mm512_fmadd_ps(t2, poly, _mm512_set1_ps(0.2f));
	poly = _mm512_fmadd_ps(t2, poly, _mm512_set1_ps(0.333333333f));
	__m512 bracket = _mm512_fmadd_ps(t2, poly, one);
	__m512 lg_m =
		_mm512_mul_ps(_mm512_mul_ps(t, _mm512_set1_ps(2.0f)), bracket);

	return _mm512_fmadd_ps(_mm512_cvtepi32_ps(exp_int), _mm512_set1_ps(LN2),
						   lg_m);
}

static inline __m512 __attribute__((always_inline)) _mm512_sin_ps(__m512 x)
{
	__m512 nf = _mm512_add_ps(_mm512_mul_ps(x, _mm512_set1_ps(1.0f / TWOPI)),
							  _mm512_set1_ps(0.5f));
	__m512i n = _mm512_cvttps_epi32(nf);
	__mmask16 less = _mm512_cmp_ps_mask(nf, _mm512_cvtepi32_ps(n), _CMP_LT_OQ);
	n = _mm512_add_epi32(n, _mm512_srai_epi32(_mm512_movm_epi32(less), 31));
	__m512 xr =
		_mm512_fnmadd_ps(_mm512_cvtepi32_ps(n), _mm512_set1_ps(TWOPI), x);

	__mmask16 negate = _mm512_cmp_ps_mask(xr, _mm512_setzero_ps(), _CMP_LT_OQ);
	__m512 ax = _mm512_mask_blend_ps(negate, xr,
									 _mm512_sub_ps(_mm512_setzero_ps(), xr));

	__mmask16 is_big = _mm512_cmp_ps_mask(ax, _mm512_set1_ps(PI), _CMP_GT_OQ);
	ax = _mm512_mask_blend_ps(is_big, ax,
							  _mm512_sub_ps(_mm512_set1_ps(TWOPI), ax));
	__mmask16 is_hi =
		_mm512_cmp_ps_mask(ax, _mm512_set1_ps(HALFPI), _CMP_GT_OQ);
	ax = _mm512_mask_blend_ps(is_hi, ax, _mm512_sub_ps(_mm512_set1_ps(PI), ax));

	__m512 x2 = _mm512_mul_ps(ax, ax);
	__m512 x3 = _mm512_mul_ps(x2, ax);
	__m512 x5 = _mm512_mul_ps(x3, x2);
	__m512 x7 = _mm512_mul_ps(x5, x2);
	__m512 x9 = _mm512_mul_ps(x7, x2);
	__m512 result = ax;
	result = _mm512_fnmadd_ps(x3, _mm512_set1_ps(0.1666666667f), result);
	result = _mm512_fmadd_ps(x5, _mm512_set1_ps(0.008333333333f), result);
	result = _mm512_fnmadd_ps(x7, _mm512_set1_ps(0.0001984126984f), result);
	result = _mm512_fmadd_ps(x9, _mm512_set1_ps(0.000002755731922f), result);

	return _mm512_mask_blend_ps(negate, result,
								_mm512_sub_ps(_mm512_setzero_ps(), result));
}

void fun_math_silu_f32(const float *x, float *out, size_t n)
{
	size_t i = 0;
	for (; i + 16 <= n; i += 16)
		_mm512_storeu_ps(out + i, _mm512_silu_ps(_mm512_loadu_ps(x + i)));
	for (; i < n; i++)
		out[i] = fun_math_silu(x[i]);
}

void fun_math_swiglu_f32_cpu(const float *gate, const float *up, float *out,
							 size_t n)
{
	size_t i = 0;
	for (; i + 16 <= n; i += 16) {
		__m512 g = _mm512_loadu_ps(gate + i);
		__m512 u = _mm512_loadu_ps(up + i);
		__m512 sg = _mm512_silu_ps(g);
		_mm512_storeu_ps(out + i, _mm512_mul_ps(sg, u));
	}
	for (; i < n; i++) {
		float s = fun_math_sigmoid(gate[i]);
		out[i] = gate[i] * s * up[i];
	}
}

void fun_math_rms_norm_f32_cpu(const float *x, const float *weight, float *out,
							   size_t n, float eps)
{
	__m512 ss = _mm512_setzero_ps();
	size_t i = 0;
	for (; i + 16 <= n; i += 16) {
		__m512 v = _mm512_loadu_ps(x + i);
		ss = _mm512_fmadd_ps(v, v, ss);
	}
	float ss_val = _mm512_reduce_add_ps(ss);
	for (; i < n; i++)
		ss_val += x[i] * x[i];

	float scale = 1.0f / __builtin_sqrtf(ss_val / (float)n + eps);
	__m512 s512 = _mm512_set1_ps(scale);
	for (i = 0; i + 16 <= n; i += 16) {
		__m512 v = _mm512_loadu_ps(x + i);
		__m512 w = _mm512_loadu_ps(weight + i);
		_mm512_storeu_ps(out + i, _mm512_mul_ps(_mm512_mul_ps(v, s512), w));
	}
	for (; i < n; i++)
		out[i] = x[i] * scale * weight[i];
}

void fun_math_softmax_f32_cpu(float *x, size_t n)
{
	if (n == 0)
		return;

	size_t i;
	float m = x[0];
	for (i = 1; i < n; i++)
		if (x[i] > m)
			m = x[i];

	float sum = 0.0f;
	__m512 m512 = _mm512_set1_ps(m);
	__m512 lo = _mm512_set1_ps(-80.0f);
	for (i = 0; i + 16 <= n; i += 16) {
		__m512 v = _mm512_loadu_ps(x + i);
		v = _mm512_sub_ps(v, m512);
		v = _mm512_max_ps(v, lo);
		__m512 e = _mm512_exp_ps(v);
		_mm512_storeu_ps(x + i, e);
		sum += _mm512_reduce_add_ps(e);
	}
	for (; i < n; i++) {
		float e = fun_math_exp(x[i] - m);
		if (e != e)
			e = 0.0f;
		x[i] = e;
		sum += e;
	}

	for (i = 0; i + 16 <= n; i += 16) {
		__m512 v = _mm512_loadu_ps(x + i);
		_mm512_storeu_ps(x + i, _mm512_div_ps(v, _mm512_set1_ps(sum)));
	}
	for (; i < n; i++)
		x[i] /= sum;
}

void fun_math_matrix_vector_f32_cpu(const float *w, const float *x,
									const float *bias, float *out, size_t rows,
									size_t cols)
{
	for (size_t r = 0; r < rows; r++) {
		const float *wr = w + r * cols;
		__m512 sum = _mm512_setzero_ps();
		size_t c = 0;
		for (; c + 16 <= cols; c += 16)
			sum = _mm512_fmadd_ps(_mm512_loadu_ps(wr + c),
								  _mm512_loadu_ps(x + c), sum);
		float s = _mm512_reduce_add_ps(sum);
		for (; c < cols; c++)
			s += wr[c] * x[c];
		out[r] = (bias ? bias[r] : 0.0f) + s;
	}
}

float fun_math_dot_f32_cpu(const float *a, const float *b, size_t n)
{
	__m512 sum = _mm512_setzero_ps();
	size_t i = 0;
	for (; i + 16 <= n; i += 16)
		sum = _mm512_fmadd_ps(_mm512_loadu_ps(a + i), _mm512_loadu_ps(b + i),
							  sum);
	float s = _mm512_reduce_add_ps(sum);
	for (; i < n; i++)
		s += a[i] * b[i];
	return s;
}

void fun_math_exp_f32(const float *x, float *out, size_t n)
{
	size_t i = 0;
	__m512 lo = _mm512_set1_ps(-87.3365f);
	__m512 hi = _mm512_set1_ps(88.7228f);
	__m512 pinf = _mm512_castsi512_ps(_mm512_set1_epi32(0x7F800000));
	__m512 pnan = _mm512_castsi512_ps(_mm512_set1_epi32(0x7FC00000));
	for (; i + 16 <= n; i += 16) {
		__m512 v = _mm512_loadu_ps(x + i);
		__mmask16 overflow = _mm512_cmp_ps_mask(v, hi, _CMP_GE_OQ);
		__mmask16 underflow = _mm512_cmp_ps_mask(v, lo, _CMP_LE_OQ);
		__mmask16 is_nan = _mm512_cmp_ps_mask(v, v, _CMP_UNORD_Q);
		__m512 vc = _mm512_min_ps(_mm512_max_ps(v, lo), hi);
		__m512 e = _mm512_exp_ps(vc);
		e = _mm512_mask_blend_ps(overflow, e, pinf);
		e = _mm512_mask_blend_ps(underflow, e, _mm512_setzero_ps());
		e = _mm512_mask_blend_ps(is_nan, e, pnan);
		_mm512_storeu_ps(out + i, e);
	}
	for (; i < n; i++)
		out[i] = fun_math_exp(x[i]);
}

void fun_math_log_f32(const float *x, float *out, size_t n)
{
	size_t i = 0;
	__m512 pneginf = _mm512_castsi512_ps(_mm512_set1_epi32(0xFF800000));
	__m512 pinf = _mm512_castsi512_ps(_mm512_set1_epi32(0x7F800000));
	__m512 pnan = _mm512_castsi512_ps(_mm512_set1_epi32(0x7FC00000));
	for (; i + 16 <= n; i += 16) {
		__m512 v = _mm512_loadu_ps(x + i);
		__m512 e = _mm512_log_ps(v);
		__m512i bits = _mm512_castps_si512(v);
		__mmask16 is_zero = _mm512_cmpeq_epi32_mask(_mm512_slli_epi32(bits, 1),
													_mm512_setzero_si512());
		__mmask16 is_neg =
			_mm512_cmpgt_epi32_mask(_mm512_setzero_si512(), bits);
		__mmask16 is_pos_inf = _mm512_cmpeq_epi32_mask(
			_mm512_and_si512(bits, _mm512_set1_epi32(0x7FFFFFFF)),
			_mm512_set1_epi32(0x7F800000));
		__mmask16 is_nan = _mm512_cmp_ps_mask(v, v, _CMP_UNORD_Q);
		e = _mm512_mask_blend_ps(is_zero, e, pneginf);
		e = _mm512_mask_blend_ps(is_neg, e, pnan);
		e = _mm512_mask_blend_ps(is_pos_inf, e, pinf);
		e = _mm512_mask_blend_ps(is_nan, e, pnan);
		_mm512_storeu_ps(out + i, e);
	}
	for (; i < n; i++)
		out[i] = fun_math_log(x[i]);
}

void fun_math_sin_f32(const float *x, float *out, size_t n)
{
	size_t i = 0;
	for (; i + 16 <= n; i += 16)
		_mm512_storeu_ps(out + i, _mm512_sin_ps(_mm512_loadu_ps(x + i)));
	for (; i < n; i++)
		out[i] = fun_math_sin(x[i]);
}

void fun_math_cos_f32(const float *x, float *out, size_t n)
{
	size_t i = 0;
	for (; i + 16 <= n; i += 16) {
		__m512 v =
			_mm512_add_ps(_mm512_loadu_ps(x + i), _mm512_set1_ps(HALFPI));
		_mm512_storeu_ps(out + i, _mm512_sin_ps(v));
	}
	for (; i < n; i++)
		out[i] = fun_math_cos(x[i]);
}

void fun_math_rotary_f32_cpu(const float *x, const float *cosv,
							 const float *sinv, float *out, size_t n_heads,
							 size_t half)
{
	for (size_t h = 0; h < n_heads; h++) {
		const float *xh = x + h * 2 * half;
		float *oh = out + h * 2 * half;
		size_t j = 0;
		for (; j + 16 <= half; j += 16) {
			__m512 a = _mm512_loadu_ps(xh + j);
			__m512 b = _mm512_loadu_ps(xh + j + half);
			__m512 c = _mm512_loadu_ps(cosv + j);
			__m512 s = _mm512_loadu_ps(sinv + j);
			_mm512_storeu_ps(oh + j,
							 _mm512_fmsub_ps(a, c, _mm512_mul_ps(b, s)));
			_mm512_storeu_ps(oh + j + half,
							 _mm512_fmadd_ps(a, s, _mm512_mul_ps(b, c)));
		}
		for (; j < half; j++) {
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
	__m512 s512 = _mm512_set1_ps(scale);
	for (size_t t = 0; t < n_rows; t++) {
		const float *row = x + t * row_stride;
		__m512 sum = _mm512_setzero_ps();
		size_t d = 0;
		for (; d + 16 <= row_len; d += 16)
			sum = _mm512_fmadd_ps(_mm512_loadu_ps(q + d),
								  _mm512_loadu_ps(row + d), sum);
		float s = _mm512_reduce_add_ps(sum);
		for (; d < row_len; d++)
			s += q[d] * row[d];
		out[t] = s * scale;
	}
}

void fun_math_weighted_sum_f32_cpu(const float *wgt, const float *x, float *out,
								   size_t n_rows, size_t row_len,
								   size_t row_stride)
{
	size_t d = 0;
	for (; d + 16 <= row_len; d += 16)
		_mm512_storeu_ps(out + d, _mm512_setzero_ps());
	for (; d < row_len; d++)
		out[d] = 0.0f;

	for (size_t t = 0; t < n_rows; t++) {
		__m512 w = _mm512_set1_ps(wgt[t]);
		const float *xr = x + t * row_stride;
		d = 0;
		for (; d + 16 <= row_len; d += 16) {
			__m512 acc = _mm512_loadu_ps(out + d);
			acc = _mm512_fmadd_ps(w, _mm512_loadu_ps(xr + d), acc);
			_mm512_storeu_ps(out + d, acc);
		}
		for (; d < row_len; d++)
			out[d] += wgt[t] * xr[d];
	}
}
