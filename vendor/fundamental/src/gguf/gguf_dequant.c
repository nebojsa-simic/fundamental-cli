#include "fundamental/gguf/gguf.h"
#include "fundamental/math/math.h"

#include <stdint.h>

static const float kvalues_mxfp4[16] = { 0.0f,	0.5f,  1.0f,  1.5f,
										 2.0f,	3.0f,  4.0f,  6.0f,
										 -0.0f, -0.5f, -1.0f, -1.5f,
										 -2.0f, -3.0f, -4.0f, -6.0f };

extern const uint8_t *fun_gguf_get_raw_data(const GGufFile *f);

voidResult fun_gguf_dequant_f32(GGufFile *f, String name, float *out)
{
	voidResult result;
	result.error.code = 0;
	result.error.message = NULL;

	uint64_tResult off_res = fun_gguf_get_tensor_offset(f, name);
	if (fun_error_is_error(off_res.error)) {
		result.error = off_res.error;
		return result;
	}
	uint64_tResult sz_res = fun_gguf_get_tensor_size(f, name);
	if (fun_error_is_error(sz_res.error)) {
		result.error = sz_res.error;
		return result;
	}
	uint32_tResult tp_res = fun_gguf_get_tensor_type(f, name);
	if (fun_error_is_error(tp_res.error)) {
		result.error = tp_res.error;
		return result;
	}

	if (tp_res.value != GGUF_TYPE_F32) {
		result.error.code = ERROR_CODE_GGUF_PARSE_ERROR;
		result.error.message = "Tensor is not F32";
		return result;
	}

	const float *src =
		(const float *)(fun_gguf_get_raw_data(f) + off_res.value);
	uint64_t count = sz_res.value / 4;
	for (uint64_t i = 0; i < count; i++)
		out[i] = src[i];
	return result;
}

voidResult fun_gguf_dequant_f32_range(GGufFile *f, String name,
									  uint64_t elem_start, uint64_t elem_count,
									  float *out)
{
	voidResult result;
	result.error.code = 0;
	result.error.message = NULL;

	uint64_tResult off_res = fun_gguf_get_tensor_offset(f, name);
	if (fun_error_is_error(off_res.error)) {
		result.error = off_res.error;
		return result;
	}
	uint32_tResult tp_res = fun_gguf_get_tensor_type(f, name);
	if (fun_error_is_error(tp_res.error) || tp_res.value != GGUF_TYPE_F32) {
		result.error.code = ERROR_CODE_GGUF_PARSE_ERROR;
		result.error.message = "Tensor is not F32";
		return result;
	}

	const float *src =
		(const float *)(fun_gguf_get_raw_data(f) + off_res.value);
	for (uint64_t i = 0; i < elem_count; i++)
		out[i] = src[elem_start + i];
	return result;
}

voidResult fun_gguf_dequant_q8_0(GGufFile *f, String name, float *out)
{
	voidResult result;
	result.error.code = 0;
	result.error.message = NULL;

	uint64_tResult off_res = fun_gguf_get_tensor_offset(f, name);
	if (fun_error_is_error(off_res.error)) {
		result.error = off_res.error;
		return result;
	}
	uint64_tResult sz_res = fun_gguf_get_tensor_size(f, name);
	if (fun_error_is_error(sz_res.error)) {
		result.error = sz_res.error;
		return result;
	}
	uint32_tResult tp_res = fun_gguf_get_tensor_type(f, name);
	if (fun_error_is_error(tp_res.error)) {
		result.error = tp_res.error;
		return result;
	}

	if (tp_res.value != GGUF_TYPE_Q8_0) {
		result.error.code = ERROR_CODE_GGUF_PARSE_ERROR;
		result.error.message = "Tensor is not Q8_0";
		return result;
	}

	const uint8_t *src = fun_gguf_get_raw_data(f) + off_res.value;
	uint64_t block_count = sz_res.value / 34;
	float *dst = out;

	for (uint64_t b = 0; b < block_count; b++) {
		uint16_t d_raw = (uint16_t)src[b * 34] |
						 ((uint16_t)src[b * 34 + 1] << 8);
		float d = fun_math_fp16_to_f32(d_raw);
		const int8_t *q = (const int8_t *)(src + b * 34 + 2);
		for (int j = 0; j < 32; j++)
			dst[b * 32 + j] = (float)q[j] * d;
	}
	return result;
}

voidResult fun_gguf_dequant_mxfp4(GGufFile *f, String name, float *out)
{
	voidResult result;
	result.error.code = 0;
	result.error.message = NULL;

	uint64_tResult off_res = fun_gguf_get_tensor_offset(f, name);
	if (fun_error_is_error(off_res.error)) {
		result.error = off_res.error;
		return result;
	}
	uint64_tResult sz_res = fun_gguf_get_tensor_size(f, name);
	if (fun_error_is_error(sz_res.error)) {
		result.error = sz_res.error;
		return result;
	}
	uint32_tResult tp_res = fun_gguf_get_tensor_type(f, name);
	if (fun_error_is_error(tp_res.error)) {
		result.error = tp_res.error;
		return result;
	}

	if (tp_res.value != GGUF_TYPE_MXFP4) {
		result.error.code = ERROR_CODE_GGUF_PARSE_ERROR;
		result.error.message = "Tensor is not MXFP4";
		return result;
	}

	const uint8_t *src = fun_gguf_get_raw_data(f) + off_res.value;
	uint64_t block_count = sz_res.value / 17;
	float *dst = out;

	for (uint64_t b = 0; b < block_count; b++) {
		uint8_t scale = src[b * 17];
		float scale_f = 1.0f;
		if (scale == 0)
			scale_f = 0.0f;
		else
			scale_f = *(float *)&(uint32_t){ (uint32_t)scale << 23 };

		for (int j = 0; j < 16; j++) {
			uint8_t byte = src[b * 17 + 1 + j];
			dst[b * 32 + j] = kvalues_mxfp4[byte & 0xF] * scale_f;
			dst[b * 32 + j + 16] = kvalues_mxfp4[(byte >> 4) & 0xF] * scale_f;
		}
	}
	return result;
}

voidResult fun_gguf_dequant_mxfp4_range(GGufFile *f, String name,
										uint64_t elem_start,
										uint64_t elem_count, float *out)
{
	voidResult result;
	result.error.code = 0;
	result.error.message = NULL;

	uint64_tResult off_res = fun_gguf_get_tensor_offset(f, name);
	if (fun_error_is_error(off_res.error)) {
		result.error = off_res.error;
		return result;
	}
	uint32_tResult tp_res = fun_gguf_get_tensor_type(f, name);
	if (fun_error_is_error(tp_res.error) || tp_res.value != GGUF_TYPE_MXFP4) {
		result.error.code = ERROR_CODE_GGUF_PARSE_ERROR;
		result.error.message = "Tensor is not MXFP4";
		return result;
	}

	const uint8_t *src = fun_gguf_get_raw_data(f) + off_res.value;
	uint64_t block_start = elem_start / 32;
	src += block_start * 17;
	uint64_t total_blocks = (elem_count + 31) / 32;
	float *dst = out;

	for (uint64_t b = 0; b < total_blocks; b++) {
		uint8_t scale = src[b * 17];
		float scale_f = 1.0f;
		if (scale == 0)
			scale_f = 0.0f;
		else
			scale_f = *(float *)&(uint32_t){ (uint32_t)scale << 23 };
		for (int j = 0; j < 16; j++) {
			uint8_t byte = src[b * 17 + 1 + j];
			dst[b * 32 + j] = kvalues_mxfp4[byte & 0xF] * scale_f;
			dst[b * 32 + j + 16] = kvalues_mxfp4[(byte >> 4) & 0xF] * scale_f;
		}
	}
	return result;
}
