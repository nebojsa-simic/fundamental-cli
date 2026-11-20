#ifndef LIBRARY_MATH_H
#define LIBRARY_MATH_H

#include <stddef.h>
#include <stdint.h>

float fun_math_sqrt(float x);
float fun_math_exp(float x);
float fun_math_log(float x);
float fun_math_sin(float x);
float fun_math_cos(float x);
float fun_math_tanh(float x);
float fun_math_sigmoid(float x);
float fun_math_silu(float x);
float fun_math_fp16_to_f32(uint16_t h);
uint16_t fun_math_f32_to_fp16(float x);

void fun_math_silu_f32(const float *x, float *out, size_t n);
void fun_math_rms_norm_f32_cpu(const float *x, const float *weight, float *out,
							   size_t n, float eps);
void fun_math_swiglu_f32_cpu(const float *gate, const float *up, float *out,
							 size_t n);
void fun_math_softmax_f32_cpu(float *x, size_t n);

void fun_math_matrix_vector_f32_cpu(const float *w, const float *x,
									const float *bias, float *out, size_t rows,
									size_t cols);
float fun_math_dot_f32_cpu(const float *a, const float *b, size_t n);

void fun_math_add_f32(const float *a, const float *b, float *out, size_t n);
void fun_math_sub_f32(const float *a, const float *b, float *out, size_t n);
void fun_math_mul_f32(const float *a, const float *b, float *out, size_t n);
void fun_math_div_f32(const float *a, const float *b, float *out, size_t n);
float fun_math_sum_f32(const float *x, size_t n);
float fun_math_max_f32(const float *x, size_t n);
float fun_math_min_f32(const float *x, size_t n);
float fun_math_mean_f32(const float *x, size_t n);

void fun_math_exp_f32(const float *x, float *out, size_t n);
void fun_math_log_f32(const float *x, float *out, size_t n);
void fun_math_sin_f32(const float *x, float *out, size_t n);
void fun_math_cos_f32(const float *x, float *out, size_t n);

void fun_math_rotary_f32_cpu(const float *x, const float *cosv,
							 const float *sinv, float *out, size_t n_heads,
							 size_t half);

void fun_math_rows_dot_f32_cpu(const float *q, const float *x, float *out,
							   size_t n_rows, size_t row_len, size_t row_stride,
							   float scale);
void fun_math_weighted_sum_f32_cpu(const float *wgt, const float *x, float *out,
								   size_t n_rows, size_t row_len,
								   size_t row_stride);

#define FUN_MATH_MXFP4_BLOCK_ELEMS 32u
#define FUN_MATH_MXFP4_BLOCK_BYTES 17u

/* Fused MXFP4 matrix-vector product: out[r] = sum_d w[r][d] * x[d] + b[r].
 * The weight matrix is stored in GGUF MXFP4 blocks: 32 elements per 17-byte
 * block (1 E8M0 scale byte followed by 16 nibble bytes). The E2M1 kvalue
 * table is {0, 0.5, 1, 1.5, 2, 3, 4, 6, -0, -0.5, -1, -1.5, -2, -3, -4, -6}
 * and a scale byte of 0 decodes to 0.0f, otherwise the byte shifted into the
 * exponent field of a float. row_len MUST be a multiple of 32. */
void fun_math_matrix_vector_mxfp4_f32_cpu(const uint8_t *w, const float *x,
										  const float *bias, float *out,
										  size_t rows, size_t cols);
void fun_math_matrix_vector_mxfp4_f32_strided_cpu(const uint8_t *w,
												  const float *x,
												  const float *bias, float *out,
												  size_t rows, size_t cols,
												  size_t row_stride_bytes);

#define FUN_MATH_Q8_BLOCK_ELEMS 32u
#define FUN_MATH_Q8_BLOCK_BYTES 34u

/* Q8_0 format: 34 bytes per 32-element block (2-byte fp16 scale + 32 int8
 * values).  dequant_row decodes n elements into out.  matrix_vector computes
 * out[r] = sum_d w[r][d] * x[d] for rows rows, cols columns (no bias). */
void fun_math_q8_dequant_row_f32_cpu(const uint8_t *src, float *out, size_t n);
void fun_math_q8_matrix_vector_f32_cpu(const uint8_t *w, const float *x,
									   float *out, size_t rows, size_t cols);

void fun_math_init(void);
int fun_math_has_sse2(void);
int fun_math_has_avx(void);
int fun_math_has_avx2(void);
int fun_math_has_avx512f(void);

#endif
