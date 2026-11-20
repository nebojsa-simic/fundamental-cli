#include "fundamental/math/math_kernels.h"

static const FunMathBackend *g_backend = NULL;

/* Per-dtype descriptor: element size plus load/store conversion between the
 * tensor's storage format and the f32 compute type. Every op computes in f32;
 * F16 and F64 are storage formats narrowed (or widened) through load/store. */
typedef struct {
	size_t elem_size;
	float (*load)(const void *);
	void (*store)(void *, float);
} FunMathDtypeDesc;

static float _load_f16(const void *p)
{
	return fun_math_fp16_to_f32(*(const uint16_t *)p);
}

static void _store_f16(void *p, float v)
{
	*(uint16_t *)p = fun_math_f32_to_fp16(v);
}

static float _load_f32(const void *p)
{
	return *(const float *)p;
}

static void _store_f32(void *p, float v)
{
	*(float *)p = v;
}

static float _load_f64(const void *p)
{
	return (float)*(const double *)p;
}

static void _store_f64(void *p, float v)
{
	*(double *)p = (double)v;
}

static const FunMathDtypeDesc g_dtype_desc[FUN_MATH_DTYPE_COUNT] = {
	[FUN_MATH_DTYPE_F32] = { sizeof(float), _load_f32, _store_f32 },
	[FUN_MATH_DTYPE_F16] = { sizeof(uint16_t), _load_f16, _store_f16 },
	[FUN_MATH_DTYPE_F64] = { sizeof(double), _load_f64, _store_f64 },
};

static float _unary_f32(FunMathOp op, float v)
{
	switch (op) {
	case FUN_MATH_OP_SQRT:
		return fun_math_sqrt(v);
	case FUN_MATH_OP_EXP:
		return fun_math_exp(v);
	case FUN_MATH_OP_LOG:
		return fun_math_log(v);
	case FUN_MATH_OP_SIN:
		return fun_math_sin(v);
	case FUN_MATH_OP_COS:
		return fun_math_cos(v);
	case FUN_MATH_OP_TANH:
		return fun_math_tanh(v);
	case FUN_MATH_OP_SIGMOID:
		return fun_math_sigmoid(v);
	case FUN_MATH_OP_SILU:
		return fun_math_silu(v);
	default:
		return v;
	}
}

static float _binary_f32(FunMathOp op, float a, float b)
{
	switch (op) {
	case FUN_MATH_OP_ADD:
		return a + b;
	case FUN_MATH_OP_SUB:
		return a - b;
	case FUN_MATH_OP_MUL:
		return a * b;
	case FUN_MATH_OP_DIV:
		return a / b;
	default:
		return a;
	}
}

static void _cpu_unary(FunMathOp op, FunMathDtype dtype,
					   const FunMathTensor *in, FunMathTensor *out)
{
	if ((size_t)dtype >= FUN_MATH_DTYPE_COUNT)
		return;
	const FunMathDtypeDesc *d = &g_dtype_desc[dtype];

	if (dtype == FUN_MATH_DTYPE_F32 && in->stride == 1 && out->stride == 1) {
		const float *x = (const float *)in->data;
		float *y = (float *)out->data;
		switch (op) {
		case FUN_MATH_OP_EXP:
			fun_math_exp_f32(x, y, in->n);
			return;
		case FUN_MATH_OP_LOG:
			fun_math_log_f32(x, y, in->n);
			return;
		case FUN_MATH_OP_SIN:
			fun_math_sin_f32(x, y, in->n);
			return;
		case FUN_MATH_OP_COS:
			fun_math_cos_f32(x, y, in->n);
			return;
		case FUN_MATH_OP_SILU:
			fun_math_silu_f32(x, y, in->n);
			return;
		default:
			break;
		}
	}

	const uint8_t *x = (const uint8_t *)in->data;
	uint8_t *y = (uint8_t *)out->data;
	for (size_t i = 0; i < in->n; i++)
		d->store(y + i * out->stride * d->elem_size,
				 _unary_f32(op, d->load(x + i * in->stride * d->elem_size)));
}

static void _cpu_binary(FunMathOp op, FunMathDtype dtype,
						const FunMathTensor *a, const FunMathTensor *b,
						FunMathTensor *out)
{
	if ((size_t)dtype >= FUN_MATH_DTYPE_COUNT)
		return;
	const FunMathDtypeDesc *d = &g_dtype_desc[dtype];

	if (dtype == FUN_MATH_DTYPE_F32 && a->stride == 1 && b->stride == 1 &&
		out->stride == 1) {
		const float *pa = (const float *)a->data;
		const float *pb = (const float *)b->data;
		float *po = (float *)out->data;
		switch (op) {
		case FUN_MATH_OP_ADD:
			fun_math_add_f32(pa, pb, po, a->n);
			break;
		case FUN_MATH_OP_SUB:
			fun_math_sub_f32(pa, pb, po, a->n);
			break;
		case FUN_MATH_OP_MUL:
			fun_math_mul_f32(pa, pb, po, a->n);
			break;
		case FUN_MATH_OP_DIV:
			fun_math_div_f32(pa, pb, po, a->n);
			break;
		default:
			break;
		}
		return;
	}

	const uint8_t *pa = (const uint8_t *)a->data;
	const uint8_t *pb = (const uint8_t *)b->data;
	uint8_t *po = (uint8_t *)out->data;
	for (size_t i = 0; i < a->n; i++) {
		float x = d->load(pa + i * a->stride * d->elem_size);
		float y = d->load(pb + i * b->stride * d->elem_size);
		d->store(po + i * out->stride * d->elem_size, _binary_f32(op, x, y));
	}
}

static void _cpu_reduce(FunMathOp op, FunMathDtype dtype,
						const FunMathTensor *in, void *out)
{
	if ((size_t)dtype >= FUN_MATH_DTYPE_COUNT)
		return;
	const FunMathDtypeDesc *d = &g_dtype_desc[dtype];

	if (dtype == FUN_MATH_DTYPE_F32 && in->stride == 1) {
		const float *x = (const float *)in->data;
		float *o = (float *)out;
		switch (op) {
		case FUN_MATH_OP_SUM:
			*o = fun_math_sum_f32(x, in->n);
			break;
		case FUN_MATH_OP_MAX:
			*o = fun_math_max_f32(x, in->n);
			break;
		case FUN_MATH_OP_MIN:
			*o = fun_math_min_f32(x, in->n);
			break;
		case FUN_MATH_OP_MEAN:
			*o = fun_math_mean_f32(x, in->n);
			break;
		default:
			break;
		}
		return;
	}

	const uint8_t *x = (const uint8_t *)in->data;
	if (op == FUN_MATH_OP_SUM || op == FUN_MATH_OP_MEAN) {
		float s = 0.0f;
		for (size_t i = 0; i < in->n; i++)
			s += d->load(x + i * in->stride * d->elem_size);
		if (op == FUN_MATH_OP_MEAN && in->n > 0)
			s /= (float)in->n;
		d->store(out, s);
	} else if (op == FUN_MATH_OP_MAX || op == FUN_MATH_OP_MIN) {
		float m = 0.0f;
		if (in->n > 0) {
			m = d->load(x);
			for (size_t i = 1; i < in->n; i++) {
				float v = d->load(x + i * in->stride * d->elem_size);
				if ((op == FUN_MATH_OP_MAX && v > m) ||
					(op == FUN_MATH_OP_MIN && v < m))
					m = v;
			}
		}
		d->store(out, m);
	}
}

static const FunMathBackend g_cpu_backend = {
	.unary = _cpu_unary,
	.binary = _cpu_binary,
	.reduce = _cpu_reduce,
	.matrix_vector_f32 = fun_math_matrix_vector_f32_cpu,
	.dot_f32 = fun_math_dot_f32_cpu,
	.rms_norm_f32 = fun_math_rms_norm_f32_cpu,
	.softmax_f32 = fun_math_softmax_f32_cpu,
	.rotary_f32 = fun_math_rotary_f32_cpu,
	.swiglu_f32 = fun_math_swiglu_f32_cpu,
	.matrix_vector_mxfp4_f32 = fun_math_matrix_vector_mxfp4_f32_cpu,
	.matrix_vector_mxfp4_f32_strided =
		fun_math_matrix_vector_mxfp4_f32_strided_cpu,
	.q8_matrix_vector_f32 = fun_math_q8_matrix_vector_f32_cpu,
	.q8_dequant_row_f32 = fun_math_q8_dequant_row_f32_cpu,
	.rows_dot_f32 = fun_math_rows_dot_f32_cpu,
	.weighted_sum_f32 = fun_math_weighted_sum_f32_cpu,
};

void fun_math_backend_set(const FunMathBackend *backend)
{
	g_backend = backend;
}

const FunMathBackend *fun_math_backend_get(void)
{
	return g_backend ? g_backend : &g_cpu_backend;
}

/* Dispatch to the current backend's `field` entry, falling back to `fallback`
 * (a `fun_math_*_cpu` kernel or a `g_cpu_backend` entry) when the backend
 * leaves that entry NULL. */
#define MATH_DISPATCH(field, fallback, ...)                     \
	do {                                                        \
		const FunMathBackend *backend = fun_math_backend_get(); \
		if (backend->field)                                     \
			backend->field(__VA_ARGS__);                        \
		else                                                    \
			fallback(__VA_ARGS__);                              \
	} while (0)

void fun_math_unary(FunMathOp op, FunMathDtype dtype, const FunMathTensor *in,
					FunMathTensor *out)
{
	MATH_DISPATCH(unary, g_cpu_backend.unary, op, dtype, in, out);
}

void fun_math_binary(FunMathOp op, FunMathDtype dtype, const FunMathTensor *a,
					 const FunMathTensor *b, FunMathTensor *out)
{
	MATH_DISPATCH(binary, g_cpu_backend.binary, op, dtype, a, b, out);
}

void fun_math_reduce(FunMathOp op, FunMathDtype dtype, const FunMathTensor *in,
					 void *out)
{
	MATH_DISPATCH(reduce, g_cpu_backend.reduce, op, dtype, in, out);
}

void fun_math_matrix_vector_f32(const float *w, const float *x,
								const float *bias, float *out, size_t rows,
								size_t cols)
{
	MATH_DISPATCH(matrix_vector_f32, fun_math_matrix_vector_f32_cpu, w, x, bias,
				  out, rows, cols);
}

float fun_math_dot_f32(const float *a, const float *b, size_t n)
{
	const FunMathBackend *backend = fun_math_backend_get();
	if (backend->dot_f32)
		return backend->dot_f32(a, b, n);
	return fun_math_dot_f32_cpu(a, b, n);
}

void fun_math_rms_norm_f32(const float *x, const float *weight, float *out,
						   size_t n, float eps)
{
	MATH_DISPATCH(rms_norm_f32, fun_math_rms_norm_f32_cpu, x, weight, out, n,
				  eps);
}

void fun_math_softmax_f32(float *x, size_t n)
{
	MATH_DISPATCH(softmax_f32, fun_math_softmax_f32_cpu, x, n);
}

void fun_math_rotary_f32(const float *x, const float *cosv, const float *sinv,
						 float *out, size_t n_heads, size_t half)
{
	MATH_DISPATCH(rotary_f32, fun_math_rotary_f32_cpu, x, cosv, sinv, out,
				  n_heads, half);
}

void fun_math_swiglu_f32(const float *gate, const float *up, float *out,
						 size_t n)
{
	MATH_DISPATCH(swiglu_f32, fun_math_swiglu_f32_cpu, gate, up, out, n);
}

void fun_math_matrix_vector_mxfp4_f32(const uint8_t *w, const float *x,
									  const float *bias, float *out,
									  size_t rows, size_t cols)
{
	MATH_DISPATCH(matrix_vector_mxfp4_f32, fun_math_matrix_vector_mxfp4_f32_cpu,
				  w, x, bias, out, rows, cols);
}

void fun_math_matrix_vector_mxfp4_f32_strided(const uint8_t *w, const float *x,
											  const float *bias, float *out,
											  size_t rows, size_t cols,
											  size_t row_stride_bytes)
{
	MATH_DISPATCH(matrix_vector_mxfp4_f32_strided,
				  fun_math_matrix_vector_mxfp4_f32_strided_cpu, w, x, bias, out,
				  rows, cols, row_stride_bytes);
}

void fun_math_q8_matrix_vector_f32(const uint8_t *w, const float *x, float *out,
								   size_t rows, size_t cols)
{
	MATH_DISPATCH(q8_matrix_vector_f32, fun_math_q8_matrix_vector_f32_cpu, w, x,
				  out, rows, cols);
}

void fun_math_q8_dequant_row_f32(const uint8_t *src, float *out, size_t n)
{
	MATH_DISPATCH(q8_dequant_row_f32, fun_math_q8_dequant_row_f32_cpu, src, out,
				  n);
}

void fun_math_rows_dot_f32(const float *q, const float *x, float *out,
						   size_t n_rows, size_t row_len, size_t row_stride,
						   float scale)
{
	MATH_DISPATCH(rows_dot_f32, fun_math_rows_dot_f32_cpu, q, x, out, n_rows,
				  row_len, row_stride, scale);
}

void fun_math_weighted_sum_f32(const float *wgt, const float *x, float *out,
							   size_t n_rows, size_t row_len, size_t row_stride)
{
	MATH_DISPATCH(weighted_sum_f32, fun_math_weighted_sum_f32_cpu, wgt, x, out,
				  n_rows, row_len, row_stride);
}
