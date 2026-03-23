#include "build/build.h"
#include "build/generator.h"
#include "fundamental/config/config.h"
#include "fundamental/file/file.h"
#include "fundamental/filesystem/filesystem.h"
#include "fundamental/async/async.h"
#include "fundamental/console/console.h"
#include "fundamental/memory/memory.h"
#include "fundamental/string/string.h"
#include "fundamental/process/process.h"

#define MAX_DIR_LISTING 4096
#define MAX_PATH_STORAGE 16384

/* Static storage for path strings discovered during source scanning.
 * Pointers in SourceScanResult.sources[] point into this buffer. */
static char path_storage[MAX_PATH_STORAGE];
static size_t path_storage_pos = 0;

/* --------------------------------------------------------------------------
 * fun.ini reader — extracts 'name' field value
 * Returns 1 on success, 0 if fun.ini absent or field not found.
 * -------------------------------------------------------------------------- */
int build_ini_read_name(char *output, size_t output_size)
{
	ConfigResult cfg = fun_config_load((String) "fun", 0, NULL);
	if (fun_error_is_error(cfg.error))
		return 0;

	StringResult name = fun_config_get_string(&cfg.value, (String) "name");
	int found = 0;
	if (fun_error_is_ok(name.error) && fun_string_length(name.value) > 0) {
		fun_string_copy(name.value, output, output_size);
		found = 1;
	}

	fun_config_destroy(&cfg.value);
	return found;
}

/* --------------------------------------------------------------------------
 * Real recursive directory scanner
 * Uses fun_filesystem_list_directory for actual OS traversal.
 * Discovered .c paths are stored in path_storage[]; sources[] holds pointers.
 * -------------------------------------------------------------------------- */
static void scan_directory_recursive(Path dir_path, SourceScanResult *result,
									 int depth)
{
	if (depth > 8)
		return;
	if (result->count >= MAX_SOURCE_FILES)
		return;

	MemoryResult listing_mem = fun_memory_allocate(MAX_DIR_LISTING);
	if (fun_error_is_error(listing_mem.error))
		return;

	fun_memory_fill(listing_mem.value, MAX_DIR_LISTING, 0);

	ErrorResult list_err =
		fun_filesystem_list_directory(dir_path, listing_mem.value);
	if (fun_error_is_error(list_err)) {
		fun_memory_free(&listing_mem.value);
		return;
	}

	char *listing = (char *)listing_mem.value;
	char *pos = listing;

	while (*pos != '\0' && result->count < MAX_SOURCE_FILES) {
		/* Find end of this entry */
		char *end = pos;
		while (*end != '\n' && *end != '\r' && *end != '\0')
			end++;

		size_t entry_len = (size_t)(end - pos);

		/* Skip empty entries */
		if (entry_len == 0) {
			pos = (*end != '\0') ? end + 1 : end;
			continue;
		}

		/* Skip hidden entries (starting with '.') */
		if (pos[0] == '.') {
			pos = (*end != '\0') ? end + 1 : end;
			continue;
		}

		/* Copy entry name to null-terminated buffer */
		char entry[256];
		voidResult sub = fun_string_substring((String)listing,
											  (size_t)(pos - listing),
											  entry_len, entry, sizeof(entry));
		if (fun_error_is_error(sub.error)) {
			pos = (*end != '\0') ? end + 1 : end;
			continue;
		}

		/* Build child path using Path join */
		const char *entry_comp[] = { entry };
		Path entry_path = { entry_comp, 1, false };
		const char *child_comps[32];
		Path child_path = { child_comps, 0, false };
		fun_path_join(dir_path, entry_path, &child_path);

		/* .c file? */
		if (entry_len > 2 && entry[entry_len - 2] == '.' &&
			entry[entry_len - 1] == 'c') {
			char full_str[512];
			ErrorResult ts_err =
				fun_path_to_string(child_path, full_str, sizeof(full_str));
			if (fun_error_is_ok(ts_err)) {
				StringLength full_len = fun_string_length((String)full_str);
				if (path_storage_pos + full_len + 1 <= MAX_PATH_STORAGE) {
					fun_string_copy((String)full_str,
									path_storage + path_storage_pos,
									MAX_PATH_STORAGE - path_storage_pos);
					result->sources[result->count] =
						(String)(path_storage + path_storage_pos);
					result->count++;
					path_storage_pos += full_len + 1;
				}
			}
		} else {
			/* Recurse into subdirectories */
			boolResult is_dir = fun_directory_exists(child_path);
			if (fun_error_is_ok(is_dir.error) && is_dir.value) {
				scan_directory_recursive(child_path, result, depth + 1);
			}
		}

		pos = (*end != '\0') ? end + 1 : end;
	}

	fun_memory_free(&listing_mem.value);
}

/* --------------------------------------------------------------------------
 * Public: scan src/ for all .c source files
 * -------------------------------------------------------------------------- */
SourceScanResult build_scan_sources(void)
{
	SourceScanResult result;
	result.count = 0;
	result.error_message = (String) "";

	/* Reset path storage for this scan */
	path_storage_pos = 0;

	const char *src_comp[] = { "src" };
	Path src_path = { src_comp, 1, false };
	scan_directory_recursive(src_path, &result, 0);

	return result;
}

/* --------------------------------------------------------------------------
 * Build space-separated source file list for GCC command
 * -------------------------------------------------------------------------- */
void build_sources_string(SourceScanResult scan_result, char *buffer,
						  size_t buffer_size)
{
	buffer[0] = '\0';
	size_t pos = 0;

	for (size_t i = 0; i < scan_result.count; i++) {
		StringLength src_len = fun_string_length(scan_result.sources[i]);
		if (pos + src_len + 2 < buffer_size) {
			fun_string_copy(scan_result.sources[i], buffer + pos,
							buffer_size - pos);
			pos += src_len;
			buffer[pos++] = ' ';
			buffer[pos] = '\0';
		}
	}
}

/* --------------------------------------------------------------------------
 * Append a string literal to ptr, advance ptr, return new ptr
 * -------------------------------------------------------------------------- */
char *build_append(char *ptr, const char *s, char *buf_end)
{
	StringLength len = fun_string_length((String)s);
	fun_string_copy((String)s, ptr, (size_t)(buf_end - ptr));
	return ptr + len;
}

/* --------------------------------------------------------------------------
 * Delete a file if it exists, so the subsequent write truncates cleanly.
 * -------------------------------------------------------------------------- */
static void build_delete_file_if_exists(const char *path)
{
	Path _del_path = { (const char *[]){ path }, 1, false };
	boolResult exists = fun_file_exists(_del_path);
	if (fun_error_is_error(exists.error) || !exists.value)
		return;

	char out_buf[64], err_buf[64];
	ProcessResult proc = { .stdout_data = out_buf,
						   .stdout_capacity = sizeof(out_buf),
						   .stderr_data = err_buf,
						   .stderr_capacity = sizeof(err_buf) };
	AsyncResult r;
	Platform platform = build_platform_get();
	if (platform.os == PLATFORM_OS_WINDOWS) {
		const char *args[] = { "cmd.exe", "/c", "del", "/f", "/q", path, NULL };
		r = fun_process_spawn("cmd.exe", args, NULL, &proc);
	} else {
		const char *args[] = { "rm", "-f", path, NULL };
		r = fun_process_spawn("rm", args, NULL, &proc);
	}
	fun_async_await(&r, -1);
	fun_process_free(&proc);
}

/* --------------------------------------------------------------------------
 * Write script buffer to file
 * -------------------------------------------------------------------------- */
BuildGenerationResult build_write_script(const char *script_path,
										 const char *content)
{
	BuildGenerationResult result;
	result.script_path = (String)script_path;

	build_delete_file_if_exists(script_path);

	StringLength total_len = fun_string_length((String)content);
	MemoryResult mem_result = fun_memory_allocate(total_len + 1);
	if (fun_error_is_error(mem_result.error)) {
		result.status = BUILD_GENERATED_ERROR;
		result.error_message = (String) "Failed to allocate memory";
		return result;
	}
	fun_string_copy((String)content, (char *)mem_result.value, total_len + 1);

	AsyncResult write_result =
		fun_write_memory_to_file((Write){ .file_path = (String)script_path,
										  .input = mem_result.value,
										  .bytes_to_write = total_len });
	fun_async_await(&write_result, -1);

	voidResult free_result = fun_memory_free(&mem_result.value);
	(void)free_result;

	if (write_result.status == ASYNC_COMPLETED) {
		result.status = BUILD_GENERATED_SUCCESS;
		result.error_message = (String) "";
	} else {
		result.status = BUILD_GENERATED_ERROR;
		result.error_message = (String) "Failed to write file";
	}

	return result;
}

/* --------------------------------------------------------------------------
 * Generate build script for current platform
 * -------------------------------------------------------------------------- */
BuildGenerationResult build_generate_current(void)
{
	SourceScanResult scan_result = build_scan_sources();

	if (scan_result.count == 0) {
		BuildGenerationResult error_result;
		error_result.status = BUILD_GENERATED_ERROR;
		error_result.error_message = (String) "No source files found";
		return error_result;
	}

	return build_generate_for_platform(scan_result);
}
