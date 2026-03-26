#include "fundamental/filesystem/filesystem.h"
#include "fundamental/filesystem/path.h"
#include <stdbool.h>
#include <stddef.h>

/* Platform functions */
int fun_platform_dir_open(const char *path, void *handle_buf);
int fun_platform_dir_read_entry(void *handle_buf, char *name_buf,
								size_t name_buf_size, char *type_out);
void fun_platform_dir_close(void *handle_buf);

/* ===================================================================
 * Internal Types
 * =================================================================== */

/* Per-depth frame: holds an open OS directory handle and the path
 * of the directory being scanned at this depth. */
typedef struct {
	char path_raw[FUN_WALK_PATH_SIZE]; /* Untouched path string */
	char path_tok[FUN_WALK_PATH_SIZE]; /* Mutable copy for component parse */
	const char *components[FUN_WALK_MAX_COMPONENTS];
	int component_count;
	int is_absolute;
	int handle_open;
	unsigned char handle[FUN_DIR_HANDLE_SIZE];
} FunWalkFrame;

/* Bulk work memory that lives in the caller-provided buffer. */
typedef struct {
	FunWalkFrame frames[FUN_WALK_MAX_DEPTH];
	char entry_path[FUN_WALK_PATH_SIZE]; /* Raw path for current entry */
	char entry_path_tok[FUN_WALK_PATH_SIZE]; /* Tokenised (modified in-place) */
	const char *entry_comps[FUN_WALK_MAX_COMPONENTS];
	char pend_path[FUN_WALK_PATH_SIZE]; /* Path saved for pending descent */
} WalkMem;

typedef char
	_walk_mem_size_check[FUN_WALK_MEMORY_SIZE >= sizeof(WalkMem) ? 1 : -1];

/* ===================================================================
 * Private Helpers
 * =================================================================== */

static void walk_zero(void *buf, size_t size)
{
	unsigned char *p = (unsigned char *)buf;
	for (size_t i = 0; i < size; i++)
		p[i] = 0;
}

static void walk_str_copy(const char *src, char *dst, size_t max)
{
	size_t i = 0;
	if (src == NULL || dst == NULL || max == 0)
		return;
	while (src[i] && i < max - 1) {
		dst[i] = src[i];
		i++;
	}
	dst[i] = '\0';
}

static size_t walk_str_len(const char *s)
{
	if (s == NULL)
		return 0;
	size_t n = 0;
	while (s[n])
		n++;
	return n;
}

/* Build child_path = base_raw + sep + name into out[out_size].
 * Returns 0 on success, -1 if buffer too small. */
static int walk_build_child_path(const char *base, const char *name, char *out,
								 size_t out_size)
{
	char sep = fun_path_separator();
	size_t base_len = walk_str_len(base);
	size_t name_len = walk_str_len(name);
	size_t need = base_len + 1 + name_len + 1;
	if (need > out_size)
		return -1;
	size_t i = 0;
	for (size_t j = 0; j < base_len; j++)
		out[i++] = base[j];
	out[i++] = sep;
	for (size_t j = 0; j < name_len; j++)
		out[i++] = name[j];
	out[i] = '\0';
	return 0;
}

/* Parse path_tok (modified in-place) into comps[max_comps].
 * Sets *count and *abs_out. */
static void walk_parse_components(char *path_tok, const char **comps,
								  int max_comps, int *count, int *abs_out)
{
	*count = 0;
	*abs_out = 0;

	if (path_tok == NULL || path_tok[0] == '\0')
		return;

	/* Detect absolute */
	if (path_tok[0] == '/') {
		*abs_out = 1;
	} else if (path_tok[1] == ':' &&
			   (path_tok[2] == '/' || path_tok[2] == '\\')) {
		*abs_out = 1;
	}

	char *p = path_tok;
	char *comp_start = p;

	/* Skip leading separator on Unix */
	if (*p == '/') {
		p++;
		comp_start = p;
	}

	while (1) {
		bool at_end = (*p == '\0');
		bool is_sep = (*p == '/' || *p == '\\');

		if (is_sep || at_end) {
			if (p > comp_start && *count < max_comps) {
				*p = '\0';
				comps[(*count)++] = comp_start;
			}
			if (at_end)
				break;
			comp_start = p + 1;
		}
		p++;
	}
}

/* Open a new frame at index new_top for dir_path. */
static bool walk_push_frame(WalkMem *mem, int new_top, const char *dir_path)
{
	FunWalkFrame *f = &mem->frames[new_top];
	walk_str_copy(dir_path, f->path_raw, FUN_WALK_PATH_SIZE);
	if (fun_platform_dir_open(f->path_raw, f->handle) != 0)
		return false;
	f->handle_open = 1;
	walk_str_copy(f->path_raw, f->path_tok, FUN_WALK_PATH_SIZE);
	walk_parse_components(f->path_tok, f->components, FUN_WALK_MAX_COMPONENTS,
						  &f->component_count, &f->is_absolute);
	return true;
}

/* ===================================================================
 * Public API
 * =================================================================== */

size_t fun_filesystem_walk_memory_size(void)
{
	return FUN_WALK_MEMORY_SIZE;
}

ErrorResult fun_filesystem_walk_init(FunWalkState *state, Memory work_mem,
									 Path root)
{
	if (state == NULL || work_mem == NULL || root.components == NULL ||
		root.count == 0)
		return ERROR_RESULT_NULL_POINTER;

	walk_zero(work_mem, FUN_WALK_MEMORY_SIZE);

	WalkMem *mem = (WalkMem *)work_mem;
	state->_mem = work_mem;
	state->_has_pend = false;

	/* Convert root Path to string */
	FunWalkFrame *f0 = &mem->frames[0];
	ErrorResult to_str =
		fun_path_to_string(root, f0->path_raw, FUN_WALK_PATH_SIZE);
	if (fun_error_is_error(to_str)) {
		state->_top = -1;
		return to_str;
	}

	/* Parse root path components */
	walk_str_copy(f0->path_raw, f0->path_tok, FUN_WALK_PATH_SIZE);
	walk_parse_components(f0->path_tok, f0->components, FUN_WALK_MAX_COMPONENTS,
						  &f0->component_count, &f0->is_absolute);

	/* Open root directory handle */
	if (fun_platform_dir_open(f0->path_raw, f0->handle) != 0) {
		state->_top = -1;
		return ERROR_RESULT_DIRECTORY_NOT_FOUND;
	}
	f0->handle_open = 1;
	state->_top = 0;

	return ERROR_RESULT_NO_ERROR;
}

boolResult fun_filesystem_walk_next(FunWalkState *state, FileEntry *entry,
									bool skip_children)
{
	boolResult result;
	result.error = ERROR_RESULT_NO_ERROR;
	result.value = false;

	if (state == NULL || entry == NULL || state->_mem == NULL) {
		result.error = ERROR_RESULT_NULL_POINTER;
		return result;
	}

	WalkMem *mem = (WalkMem *)state->_mem;

	/* Handle pending descent from the previously yielded directory */
	if (state->_has_pend) {
		state->_has_pend = false;
		if (!skip_children && state->_top + 1 < FUN_WALK_MAX_DEPTH) {
			int new_top = state->_top + 1;
			if (walk_push_frame(mem, new_top, mem->pend_path))
				state->_top = new_top;
		}
	}

	while (state->_top >= 0) {
		FunWalkFrame *frame = &mem->frames[state->_top];

		if (!frame->handle_open) {
			state->_top--;
			continue;
		}

		char name_buf[FUN_WALK_PATH_SIZE];
		char type;
		int r = fun_platform_dir_read_entry(frame->handle, name_buf,
											sizeof(name_buf), &type);

		if (r <= 0) {
			/* End of directory: close and pop */
			fun_platform_dir_close(frame->handle);
			frame->handle_open = 0;
			state->_top--;
			continue;
		}

		/* Build child path */
		if (walk_build_child_path(frame->path_raw, name_buf, mem->entry_path,
								  FUN_WALK_PATH_SIZE) != 0)
			continue; /* Path too long, skip */

		/* Parse child path into components */
		walk_str_copy(mem->entry_path, mem->entry_path_tok, FUN_WALK_PATH_SIZE);
		int comp_count = 0;
		int is_abs = 0;
		walk_parse_components(mem->entry_path_tok, mem->entry_comps,
							  FUN_WALK_MAX_COMPONENTS, &comp_count, &is_abs);

		/* Populate FileEntry */
		entry->path.components = mem->entry_comps;
		entry->path.count = (size_t)comp_count;
		entry->path.is_absolute = (bool)is_abs;
		entry->name = (comp_count > 0) ? mem->entry_comps[comp_count - 1] : "";
		entry->is_directory = (type == 'D');
		entry->depth = state->_top;

		/* If directory, save path for potential descent on next call */
		if (type == 'D') {
			walk_str_copy(mem->entry_path, mem->pend_path, FUN_WALK_PATH_SIZE);
			state->_has_pend = true;
		}

		result.value = true;
		return result;
	}

	return result; /* value = false: walk complete */
}

void fun_filesystem_walk_close(FunWalkState *state)
{
	if (state == NULL || state->_mem == NULL || state->_top < 0)
		return;

	WalkMem *mem = (WalkMem *)state->_mem;
	for (int i = 0; i <= state->_top; i++) {
		FunWalkFrame *frame = &mem->frames[i];
		if (frame->handle_open) {
			fun_platform_dir_close(frame->handle);
			frame->handle_open = 0;
		}
	}
	state->_top = -1;
}
