#include "fundamental/gguf/gguf.h"
#include "fundamental/memory/memory.h"
#include "fundamental/string/string.h"

struct GGufFile {
	const uint8_t *data;
	uint64_t mapped_size;
	uint64_t tensor_count;
	uint64_t kv_count;
	const uint8_t *kv_start;
	const uint8_t *tensor_start;
	uint64_t data_start;
	void *platform_handles[2];
};

static uint32_t read_le32(const uint8_t *p)
{
	return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
		   ((uint32_t)p[3] << 24);
}

static uint64_t read_le64(const uint8_t *p)
{
	return (uint64_t)read_le32(p) | ((uint64_t)read_le32(p + 4) << 32);
}

static const uint8_t *skip_str(const uint8_t *p, const uint8_t *end)
{
	uint64_t len = read_le64(p);
	p += 8;
	if (p + len > end)
		return NULL;
	return p + len;
}

static const uint8_t *skip_value(const uint8_t *p, const uint8_t *end,
								 uint32_t vtype)
{
	switch (vtype) {
	case 0:
	case 1:
	case 7:
		p += 1;
		break;
	case 2:
	case 3:
		p += 2;
		break;
	case 4:
	case 5:
	case 6:
		p += 4;
		break;
	case 8:
		return skip_str(p, end);
	case 9: {
		uint32_t et = read_le32(p);
		p += 4;
		uint64_t n = read_le64(p);
		p += 8;
		if (et == 8) {
			for (uint64_t j = 0; j < n; j++) {
				p = skip_str(p, end);
				if (!p)
					return NULL;
			}
			return p;
		}
		size_t sz = (et == 0 || et == 1 || et == 7)	   ? 1 :
					(et == 2 || et == 3)			   ? 2 :
					(et == 4 || et == 5 || et == 6)	   ? 4 :
					(et == 10 || et == 11 || et == 12) ? 8 :
														 0;
		p += n * sz;
		break;
	}
	case 10:
	case 11:
	case 12:
		p += 8;
		break;
	}
	return p;
}

static const uint8_t *find_kv(const GGufFile *f, String key, uint32_t *out_type)
{
	const uint8_t *p = f->kv_start;
	const uint8_t *end = f->data + f->mapped_size;
	uint64_t key_len = (uint64_t)fun_string_length(key);

	for (uint64_t i = 0; i < f->kv_count; i++) {
		uint64_t klen = read_le64(p);
		p += 8;
		if (p + klen > end)
			break;
		if (klen == key_len) {
			int match = 1;
			for (uint64_t j = 0; j < klen; j++) {
				if (p[j] != (uint8_t)key[j]) {
					match = 0;
					break;
				}
			}
			if (match) {
				p += klen;
				*out_type = read_le32(p);
				return p + 4;
			}
		}
		p += klen;
		uint32_t vt = read_le32(p);
		p += 4;
		p = skip_value(p, end, vt);
		if (!p)
			break;
	}
	return NULL;
}

static const uint8_t *find_tensor_info(const GGufFile *f, String name,
									   uint64_t *out_offset, uint64_t *out_size,
									   uint32_t *out_type)
{
	const uint8_t *p = f->tensor_start;
	const uint8_t *end = f->data + f->mapped_size;
	uint64_t name_len = (uint64_t)fun_string_length(name);

	for (uint64_t i = 0; i < f->tensor_count; i++) {
		uint64_t nlen = read_le64(p);
		p += 8;
		if (p + nlen > end)
			break;
		if (nlen == name_len) {
			int match = 1;
			for (uint64_t j = 0; j < nlen; j++) {
				if (p[j] != (uint8_t)name[j]) {
					match = 0;
					break;
				}
			}
			if (match) {
				p += nlen;
				uint32_t ndims = read_le32(p);
				p += 4;
				uint64_t total = 1;
				for (uint32_t d = 0; d < ndims; d++) {
					total *= read_le64(p);
					p += 8;
				}
				uint32_t ttype = read_le32(p);
				p += 4;
				uint64_t off = read_le64(p);
				p += 8;
				*out_offset = off;
				*out_type = ttype;
				uint64_t elem_size = 4;
				if (ttype == GGUF_TYPE_F16)
					elem_size = 2;
				else if (ttype == GGUF_TYPE_Q8_0)
					elem_size = 34;
				else if (ttype == GGUF_TYPE_MXFP4)
					elem_size = 17;
				*out_size = total * elem_size / 32 * 34 + (total % 32 ? 34 : 0);
				if (ttype == GGUF_TYPE_MXFP4)
					*out_size = total / 32 * 17 + (total % 32 ? 17 : 0);
				if (ttype == GGUF_TYPE_F32)
					*out_size = total * 4;
				if (ttype == GGUF_TYPE_F16)
					*out_size = total * 2;
				if (ttype == GGUF_TYPE_Q8_0) {
					uint64_t blocks = (total + 31) / 32;
					*out_size = blocks * 34;
				}
				return p;
			}
		}
		p += nlen;
		uint32_t ndims = read_le32(p);
		p += 4;
		for (uint32_t d = 0; d < ndims; d++)
			p += 8;
		p += 4;
		p += 8;
	}
	return NULL;
}

extern void *_gguf_platform_open(const char *path, uint64_t *out_size,
								 void *handles[2]);
extern void _gguf_platform_close(void *data, void *handles[2]);

GGufFileHandleResult fun_gguf_open(String path)
{
	GGufFileHandleResult result;
	result.error.code = ERROR_CODE_NO_ERROR;
	result.error.message = NULL;

	GGufFile *f = (GGufFile *)fun_memory_allocate(sizeof(GGufFile)).value;
	if (!f) {
		result.error.code = ERROR_CODE_NULL_POINTER;
		result.error.message = "Failed to allocate GGufFile";
		result.value = NULL;
		return result;
	}

	uint64_t size = 0;
	void *data = _gguf_platform_open(path, &size, f->platform_handles);
	if (!data) {
		fun_memory_free((Memory *)&f);
		result.error.code = ERROR_CODE_GGUF_NOT_FOUND;
		result.error.message = "Failed to open or mmap file";
		result.value = NULL;
		return result;
	}

	const uint8_t *p = (const uint8_t *)data;
	f->data = p;
	f->mapped_size = size;

	if (size < 24) {
		_gguf_platform_close((void *)f->data, f->platform_handles);
		fun_memory_free((Memory *)&f);
		result.error.code = ERROR_CODE_GGUF_PARSE_ERROR;
		result.error.message = "File too small";
		result.value = NULL;
		return result;
	}

	uint32_t magic = read_le32(p);
	p += 4;
	if (magic != GGUF_MAGIC) {
		_gguf_platform_close((void *)f->data, f->platform_handles);
		fun_memory_free((Memory *)&f);
		result.error.code = ERROR_CODE_GGUF_INVALID_MAGIC;
		result.error.message = "Invalid GGUF magic";
		result.value = NULL;
		return result;
	}

	p += 4;
	f->tensor_count = read_le64(p);
	p += 8;
	f->kv_count = read_le64(p);
	p += 8;

	f->kv_start = p;

	const uint8_t *end = f->data + f->mapped_size;
	for (uint64_t i = 0; i < f->kv_count; i++) {
		uint64_t klen = read_le64(p);
		p += 8;
		if (p + klen > end)
			break;
		p += klen;
		uint32_t vt = read_le32(p);
		p += 4;
		p = skip_value(p, end, vt);
		if (!p)
			break;
	}

	{
		const uint8_t *tp = p;
		while (tp < end && *tp == 0)
			tp++;
		p = tp;
	}

	f->tensor_start = p;

	{
		const uint8_t *tp = p;
		for (uint64_t i = 0; i < f->tensor_count; i++) {
			uint64_t nlen = read_le64(tp);
			tp += 8;
			if (tp + nlen > end)
				break;
			tp += nlen;
			uint32_t ndims = read_le32(tp);
			tp += 4;
			if (tp + (uint64_t)ndims * 8 > end)
				break;
			tp += (uint64_t)ndims * 8;
			tp += 4;
			tp += 8;
		}
		uint64_t meta_end = (uint64_t)(tp - f->data);
		uint64_t alignment =
			(uint64_t)fun_gguf_get_metadata_u32(f, "general.alignment").value;
		if (alignment == 0)
			alignment = 32;
		uint64_t pad = alignment - meta_end % alignment;
		if (pad == alignment)
			pad = 0;
		f->data_start = meta_end + pad;
	}

	result.value = f;
	return result;
}

void fun_gguf_close(GGufFile *file)
{
	if (!file)
		return;
	_gguf_platform_close((void *)file->data, file->platform_handles);
	fun_memory_free((Memory *)&file);
}

StringResult fun_gguf_get_metadata_string(GGufFile *f, String key)
{
	StringResult result;
	result.error.code = ERROR_CODE_NO_ERROR;
	result.error.message = NULL;

	uint32_t vt = 0;
	const uint8_t *vp = find_kv(f, key, &vt);
	if (!vp || vt != 8) {
		result.error.code = ERROR_CODE_GGUF_KEY_NOT_FOUND;
		result.error.message = "Key not found or wrong type";
		result.value = NULL;
		return result;
	}
	uint64_t vlen = read_le64(vp);
	vp += 8;
	result.value = (String)vp;
	return result;
}

int32_tResult fun_gguf_get_metadata_i32(GGufFile *f, String key)
{
	int32_tResult result;
	result.error.code = ERROR_CODE_NO_ERROR;
	result.error.message = NULL;

	uint32_t vt = 0;
	const uint8_t *vp = find_kv(f, key, &vt);
	if (!vp || vt != 5) {
		result.error.code = ERROR_CODE_GGUF_KEY_NOT_FOUND;
		result.error.message = "Key not found or wrong type";
		result.value = 0;
		return result;
	}
	uint32_t raw = read_le32(vp);
	result.value = (int32_t)raw;
	return result;
}

uint32_tResult fun_gguf_get_metadata_u32(GGufFile *f, String key)
{
	uint32_tResult result;
	result.error.code = ERROR_CODE_NO_ERROR;
	result.error.message = NULL;

	uint32_t vt = 0;
	const uint8_t *vp = find_kv(f, key, &vt);
	if (!vp || vt != 4) {
		result.error.code = ERROR_CODE_GGUF_KEY_NOT_FOUND;
		result.error.message = "Key not found or wrong type";
		result.value = 0;
		return result;
	}
	result.value = read_le32(vp);
	return result;
}

floatResult fun_gguf_get_metadata_f32(GGufFile *f, String key)
{
	floatResult result;
	result.error.code = ERROR_CODE_NO_ERROR;
	result.error.message = NULL;

	uint32_t vt = 0;
	const uint8_t *vp = find_kv(f, key, &vt);
	if (!vp || vt != 6) {
		result.error.code = ERROR_CODE_GGUF_KEY_NOT_FOUND;
		result.error.message = "Key not found or wrong type";
		result.value = 0.0f;
		return result;
	}
	uint32_t raw = read_le32(vp);
	result.value = *(float *)&raw;
	return result;
}

int32_tResult fun_gguf_get_tensor_count(GGufFile *f)
{
	int32_tResult result;
	result.error.code = ERROR_CODE_NO_ERROR;
	result.error.message = NULL;
	result.value = (int32_t)f->tensor_count;
	return result;
}

uint64_tResult fun_gguf_get_tensor_offset(GGufFile *f, String name)
{
	uint64_tResult result;
	result.error.code = ERROR_CODE_NO_ERROR;
	result.error.message = NULL;

	uint64_t off = 0, sz = 0;
	uint32_t tp = 0;
	if (!find_tensor_info(f, name, &off, &sz, &tp)) {
		result.error.code = ERROR_CODE_GGUF_TENSOR_NOT_FOUND;
		result.error.message = "Tensor not found";
		result.value = 0;
		return result;
	}
	result.value = off;
	return result;
}

uint64_tResult fun_gguf_get_tensor_size(GGufFile *f, String name)
{
	uint64_tResult result;
	result.error.code = ERROR_CODE_NO_ERROR;
	result.error.message = NULL;

	uint64_t off = 0, sz = 0;
	uint32_t tp = 0;
	if (!find_tensor_info(f, name, &off, &sz, &tp)) {
		result.error.code = ERROR_CODE_GGUF_TENSOR_NOT_FOUND;
		result.error.message = "Tensor not found";
		result.value = 0;
		return result;
	}
	result.value = sz;
	return result;
}

uint32_tResult fun_gguf_get_tensor_type(GGufFile *f, String name)
{
	uint32_tResult result;
	result.error.code = ERROR_CODE_NO_ERROR;
	result.error.message = NULL;

	uint64_t off = 0, sz = 0;
	uint32_t tp = 0;
	if (!find_tensor_info(f, name, &off, &sz, &tp)) {
		result.error.code = ERROR_CODE_GGUF_TENSOR_NOT_FOUND;
		result.error.message = "Tensor not found";
		result.value = 0;
		return result;
	}
	result.value = tp;
	return result;
}

const uint8_t *fun_gguf_get_raw_data(const GGufFile *f)
{
	return f->data + f->data_start;
}

uint64_t fun_gguf_get_raw_size(const GGufFile *f)
{
	return f->mapped_size - f->data_start;
}

const uint8_t *fun_gguf_get_file_base(const GGufFile *f)
{
	return f->data;
}

uint64_t fun_gguf_get_data_start(const GGufFile *f)
{
	return f->data_start;
}

const uint8_t *fun_gguf_get_kv_start(const GGufFile *f, uint64_t *count)
{
	*count = f->kv_count;
	return f->kv_start;
}

StringResult fun_gguf_get_token_string(const GGufFile *f, uint32_t index,
									   uint64_t *out_len)
{
	StringResult result;
	result.error.code = ERROR_CODE_NO_ERROR;
	result.error.message = NULL;
	result.value = NULL;
	if (out_len)
		*out_len = 0;

	static const uint8_t *cached_vp = NULL;
	static uint64_t cached_total = 0;
	static uint64_t cached_pos = 0;
	static const uint8_t *cached_p = NULL;
	static const GGufFile *cached_f = NULL;

	if (cached_f != f || cached_vp == NULL) {
		uint32_t vt = 0;
		cached_vp = find_kv(f, "tokenizer.ggml.tokens", &vt);
		if (!cached_vp || vt != 9) {
			result.error.code = ERROR_CODE_GGUF_KEY_NOT_FOUND;
			result.error.message = "tokenizer.ggml.tokens not found";
			return result;
		}
		cached_total = read_le64(cached_vp + 4);
		cached_p = cached_vp + 12;
		cached_pos = 0;
		cached_f = f;
	}

	if (index >= cached_total) {
		result.error.code = ERROR_CODE_GGUF_PARSE_ERROR;
		result.error.message = "Token index out of bounds";
		return result;
	}

	const uint8_t *p = cached_p;
	uint64_t pos = cached_pos;
	const uint8_t *end = f->data + f->mapped_size;

	if (index == pos) {
	} else if (index > pos) {
		for (uint64_t i = pos; i < index; i++) {
			uint64_t len = read_le64(p);
			p += 8;
			if (p + len > end) {
				result.error.code = ERROR_CODE_GGUF_PARSE_ERROR;
				result.error.message = "Token overflow";
				return result;
			}
			p += len;
		}
	} else {
		p = cached_vp + 12;
		pos = 0;
		cached_p = p;
		for (uint64_t i = 0; i < index; i++) {
			uint64_t len = read_le64(p);
			p += 8;
			p += len;
		}
	}

	uint64_t len = read_le64(p);
	p += 8;
	if (p + len > end) {
		result.error.code = ERROR_CODE_GGUF_PARSE_ERROR;
		result.error.message = "Token overflow";
		return result;
	}

	cached_p = p + len;
	cached_pos = index + 1;
	result.value = (String)p;
	if (out_len)
		*out_len = len;
	return result;
}
