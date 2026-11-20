#ifndef LIBRARY_GGUF_H
#define LIBRARY_GGUF_H

#include <stdint.h>
#include "../error/error.h"
#include "../string/string.h"

#define GGUF_MAGIC 0x46554747

#define GGUF_TYPE_F32 0
#define GGUF_TYPE_F16 1
#define GGUF_TYPE_Q8_0 8
#define GGUF_TYPE_Q8_1 9
#define GGUF_TYPE_Q2_K 10
#define GGUF_TYPE_Q3_K 11
#define GGUF_TYPE_Q4_K 12
#define GGUF_TYPE_Q5_K 13
#define GGUF_TYPE_Q6_K 14
#define GGUF_TYPE_MXFP4 39

#define ERROR_CODE_GGUF_NOT_FOUND 300
#define ERROR_CODE_GGUF_INVALID_MAGIC 301
#define ERROR_CODE_GGUF_PARSE_ERROR 302
#define ERROR_CODE_GGUF_TENSOR_NOT_FOUND 303
#define ERROR_CODE_GGUF_KEY_NOT_FOUND 304

typedef struct GGufFile GGufFile;
typedef GGufFile *GGufFileHandle;
DEFINE_RESULT_TYPE(GGufFileHandle);
DEFINE_RESULT_TYPE(String);

CanReturnError(GGufFileHandle) fun_gguf_open(String path);
void fun_gguf_close(GGufFile *file);

CanReturnError(String) fun_gguf_get_metadata_string(GGufFile *f, String key);
CanReturnError(int32_t) fun_gguf_get_metadata_i32(GGufFile *f, String key);
CanReturnError(uint32_t) fun_gguf_get_metadata_u32(GGufFile *f, String key);
CanReturnError(float) fun_gguf_get_metadata_f32(GGufFile *f, String key);
CanReturnError(int32_t) fun_gguf_get_tensor_count(GGufFile *f);
const uint8_t *fun_gguf_get_raw_data(const GGufFile *f);
uint64_t fun_gguf_get_raw_size(const GGufFile *f);
const uint8_t *fun_gguf_get_file_base(const GGufFile *f);
uint64_t fun_gguf_get_data_start(const GGufFile *f);
const uint8_t *fun_gguf_get_kv_start(const GGufFile *f, uint64_t *count);
CanReturnError(String)
	fun_gguf_get_token_string(const GGufFile *f, uint32_t index,
							  uint64_t *out_len);

CanReturnError(uint64_t) fun_gguf_get_tensor_offset(GGufFile *f, String name);
CanReturnError(uint64_t) fun_gguf_get_tensor_size(GGufFile *f, String name);
CanReturnError(uint32_t) fun_gguf_get_tensor_type(GGufFile *f, String name);

CanReturnError(void) fun_gguf_dequant_f32(GGufFile *f, String name, float *out);
CanReturnError(void)
	fun_gguf_dequant_f32_range(GGufFile *f, String name, uint64_t elem_start,
							   uint64_t elem_count, float *out);
CanReturnError(void)
	fun_gguf_dequant_q8_0(GGufFile *f, String name, float *out);
CanReturnError(void)
	fun_gguf_dequant_mxfp4(GGufFile *f, String name, float *out);
CanReturnError(void)
	fun_gguf_dequant_mxfp4_range(GGufFile *f, String name, uint64_t elem_start,
								 uint64_t elem_count, float *out);

#endif
