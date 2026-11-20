#ifndef LIBRARY_MATH_KERNELS_H
#define LIBRARY_MATH_KERNELS_H

#include <stddef.h>

#include "fundamental/math/math.h"

/* Backend dispatch layer for the math module.
 *
 * A backend is a vtable of kernel function pointers. The built-in CPU backend
 * (the default) forwards to the built-in fun_math_* kernels. A caller may
 * install a custom backend (for example a future GPU/accelerator backend) with
 * fun_math_backend_set; subsequent dispatches route through it. Passing NULL to
 * fun_math_backend_set restores the default CPU backend. Backend selection is
 * separate from, and does not affect, the link-time weak-symbol ISA dispatch of
 * the built-in CPU kernels.
 *
 * fun_math_backend_set is not thread-safe: call it during single-threaded
 * startup, before any dispatch runs. */

typedef enum {
	FUN_MATH_DTYPE_F32 = 0,
	FUN_MATH_DTYPE_F16,
	FUN_MATH_DTYPE_F64,
	FUN_MATH_DTYPE_COUNT
} FunMathDtype;

typedef enum {
	FUN_MATH_OP_SQRT = 0,
	FUN_MATH_OP_EXP,
	FUN_MATH_OP_LOG,
	FUN_MATH_OP_SIN,
	FUN_MATH_OP_COS,
	FUN_MATH_OP_TANH,
	FUN_MATH_OP_SIGMOID,
	FUN_MATH_OP_SILU,
	FUN_MATH_OP_ADD,
	FUN_MATH_OP_SUB,
	FUN_MATH_OP_MUL,
	FUN_MATH_OP_DIV,
	FUN_MATH_OP_SUM,
	FUN_MATH_OP_MAX,
	FUN_MATH_OP_MIN,
	FUN_MATH_OP_MEAN,
	FUN_MATH_OP_COUNT
} FunMathOp;

typedef struct {
	void *data;
	size_t n;
	size_t stride;
} FunMathTensor;

typedef struct FunMathBackend FunMathBackend;

struct FunMathBackend {
	void (*unary)(FunMathOp op, FunMathDtype dtype, const FunMathTensor *in,
				  FunMathTensor *out);
	void (*binary)(FunMathOp op, FunMathDtype dtype, const FunMathTensor *a,
				   const FunMathTensor *b, FunMathTensor *out);
	void (*reduce)(FunMathOp op, FunMathDtype dtype, const FunMathTensor *in,
				   void *out);
	void (*matrix_vector_f32)(const float *w, const float *x, const float *bias,
							  float *out, size_t rows, size_t cols);
	float (*dot_f32)(const float *a, const float *b, size_t n);
	void (*rms_norm_f32)(const float *x, const float *weight, float *out,
						 size_t n, float eps);
	void (*softmax_f32)(float *x, size_t n);
	void (*rotary_f32)(const float *x, const float *cosv, const float *sinv,
					   float *out, size_t n_heads, size_t half);
	void (*swiglu_f32)(const float *gate, const float *up, float *out,
					   size_t n);
	void (*matrix_vector_mxfp4_f32)(const uint8_t *w, const float *x,
									const float *bias, float *out, size_t rows,
									size_t cols);
	void (*matrix_vector_mxfp4_f32_strided)(const uint8_t *w, const float *x,
											const float *bias, float *out,
											size_t rows, size_t cols,
											size_t row_stride_bytes);
	void (*q8_matrix_vector_f32)(const uint8_t *w, const float *x, float *out,
								 size_t rows, size_t cols);
	void (*q8_dequant_row_f32)(const uint8_t *src, float *out, size_t n);
	void (*rows_dot_f32)(const float *q, const float *x, float *out,
						 size_t n_rows, size_t row_len, size_t row_stride,
						 float scale);
	void (*weighted_sum_f32)(const float *wgt, const float *x, float *out,
							 size_t n_rows, size_t row_len, size_t row_stride);
};

void fun_math_backend_set(const FunMathBackend *backend);
const FunMathBackend *fun_math_backend_get(void);

void fun_math_unary(FunMathOp op, FunMathDtype dtype, const FunMathTensor *in,
					FunMathTensor *out);
void fun_math_binary(FunMathOp op, FunMathDtype dtype, const FunMathTensor *a,
					 const FunMathTensor *b, FunMathTensor *out);
void fun_math_reduce(FunMathOp op, FunMathDtype dtype, const FunMathTensor *in,
					 void *out);

void fun_math_matrix_vector_f32(const float *w, const float *x,
								const float *bias, float *out, size_t rows,
								size_t cols);
float fun_math_dot_f32(const float *a, const float *b, size_t n);
void fun_math_rms_norm_f32(const float *x, const float *weight, float *out,
						   size_t n, float eps);
void fun_math_softmax_f32(float *x, size_t n);
void fun_math_rotary_f32(const float *x, const float *cosv, const float *sinv,
						 float *out, size_t n_heads, size_t half);

void fun_math_swiglu_f32(const float *gate, const float *up, float *out,
						 size_t n);
void fun_math_matrix_vector_mxfp4_f32(const uint8_t *w, const float *x,
									  const float *bias, float *out,
									  size_t rows, size_t cols);
void fun_math_matrix_vector_mxfp4_f32_strided(const uint8_t *w, const float *x,
											  const float *bias, float *out,
											  size_t rows, size_t cols,
											  size_t row_stride_bytes);
void fun_math_q8_matrix_vector_f32(const uint8_t *w, const float *x, float *out,
								   size_t rows, size_t cols);
void fun_math_q8_dequant_row_f32(const uint8_t *src, float *out, size_t n);
void fun_math_rows_dot_f32(const float *q, const float *x, float *out,
						   size_t n_rows, size_t row_len, size_t row_stride,
						   float scale);
void fun_math_weighted_sum_f32(const float *wgt, const float *x, float *out,
							   size_t n_rows, size_t row_len,
							   size_t row_stride);

#endif
