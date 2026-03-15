#include "generator.h"
#include "../fun/platform.h"
#include "vendor/fundamental/include/file/file.h"
#include "vendor/fundamental/include/filesystem/filesystem.h"
#include "vendor/fundamental/include/async/async.h"
#include "vendor/fundamental/include/console/console.h"
#include "vendor/fundamental/include/memory/memory.h"
#include "vendor/fundamental/include/string/string.h"

#define MAX_DIR_LISTING  4096
#define MAX_PATH_STORAGE 16384

/* Static storage for path strings discovered during source scanning.
 * Pointers in SourceScanResult.sources[] point into this buffer. */
static char   path_storage[MAX_PATH_STORAGE];
static size_t path_storage_pos = 0;

/* --------------------------------------------------------------------------
 * fun.ini reader — extracts 'name' field value
 * Returns 1 on success, 0 if fun.ini absent or field not found.
 * -------------------------------------------------------------------------- */
static int fun_ini_read_name(char *output, size_t output_size)
{
	boolResult exists = fun_file_exists((String)"fun.ini");
	if (fun_error_is_error(exists.error) || !exists.value)
		return 0;

	MemoryResult mem = fun_memory_allocate(512);
	if (fun_error_is_error(mem.error))
		return 0;

	fun_memory_fill(mem.value, 512, 0);

	/* Try decreasing read sizes — fun_read_file_in_memory requires exact
	 * byte count, so we probe from large to small until one succeeds. */
	static const size_t try_sizes[] = { 256, 128, 64, 32, 0 };
	int read_ok = 0;
	for (int t = 0; try_sizes[t] != 0; t++) {
		fun_memory_fill(mem.value, 512, 0);
		AsyncResult r = fun_read_file_in_memory(
			(Read){ .file_path = (String)"fun.ini",
				    .output = mem.value,
				    .bytes_to_read = try_sizes[t] });
		fun_async_await(&r, -1);
		if (r.status == ASYNC_COMPLETED) {
			read_ok = 1;
			break;
		}
	}

	int found = 0;
	char *buf = (char *)mem.value;
	if (!read_ok) {
		fun_memory_free(&mem.value);
		return 0;
	}
	char *pos = buf;

	while (*pos != '\0') {
		/* Skip leading whitespace on this line */
		while (*pos == ' ' || *pos == '\t')
			pos++;

		/* Look for "name" keyword */
		if (pos[0] == 'n' && pos[1] == 'a' && pos[2] == 'm' && pos[3] == 'e') {
			char *p = pos + 4;
			while (*p == ' ' || *p == '\t')
				p++;
			if (*p == '=') {
				p++;
				while (*p == ' ' || *p == '\t')
					p++;
				size_t i = 0;
				while (*p != '\0' && *p != '\n' && *p != '\r' &&
				       i < output_size - 1)
					output[i++] = *p++;
				output[i] = '\0';
				/* Trim trailing whitespace */
				while (i > 0 &&
				       (output[i - 1] == ' ' || output[i - 1] == '\t'))
					output[--i] = '\0';
				if (i > 0) {
					found = 1;
					break;
				}
			}
		}

		/* Advance to next line */
		while (*pos != '\0' && *pos != '\n')
			pos++;
		if (*pos == '\n')
			pos++;
	}

	fun_memory_free(&mem.value);
	return found;
}

/* --------------------------------------------------------------------------
 * Real recursive directory scanner
 * Uses fun_filesystem_list_directory for actual OS traversal.
 * Discovered .c paths are stored in path_storage[]; sources[] holds pointers.
 * -------------------------------------------------------------------------- */
static void scan_directory_recursive(String dir_path, SourceScanResult *result,
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

		/* Skip empty or oversized entries */
		if (entry_len == 0 || entry_len >= 256) {
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
		for (size_t i = 0; i < entry_len; i++)
			entry[i] = pos[i];
		entry[entry_len] = '\0';

		/* Build full path: dir_path + sep + entry */
		char full_path[512];
		ErrorResult join_err =
			fun_path_join(dir_path, (String)entry, (OutputString)full_path);
		if (fun_error_is_error(join_err)) {
			pos = (*end != '\0') ? end + 1 : end;
			continue;
		}

		/* .c file? */
		if (entry_len > 2 && entry[entry_len - 2] == '.' &&
		    entry[entry_len - 1] == 'c') {
			StringLength full_len = fun_string_length((String)full_path);
			if (path_storage_pos + full_len + 1 <= MAX_PATH_STORAGE) {
				fun_string_copy((String)full_path,
				                path_storage + path_storage_pos);
				result->sources[result->count] =
					(String)(path_storage + path_storage_pos);
				result->count++;
				path_storage_pos += full_len + 1;
			}
		} else {
			/* Recurse into subdirectories */
			boolResult is_dir = fun_directory_exists((String)full_path);
			if (fun_error_is_ok(is_dir.error) && is_dir.value) {
				scan_directory_recursive((String)full_path, result,
				                         depth + 1);
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
	result.error_message = (String)"";

	/* Reset path storage for this scan */
	path_storage_pos = 0;

	scan_directory_recursive((String)"src", &result, 0);

	return result;
}

/* --------------------------------------------------------------------------
 * Build space-separated source file list for GCC command
 * -------------------------------------------------------------------------- */
static void build_sources_string(SourceScanResult scan_result, char *buffer,
                                 size_t buffer_size)
{
	buffer[0] = '\0';
	size_t pos = 0;

	for (size_t i = 0; i < scan_result.count; i++) {
		StringLength src_len = fun_string_length(scan_result.sources[i]);
		if (pos + src_len + 2 < buffer_size) {
			fun_string_copy(scan_result.sources[i], buffer + pos);
			pos += src_len;
			buffer[pos++] = ' ';
			buffer[pos] = '\0';
		}
	}
}

/* --------------------------------------------------------------------------
 * Append a string literal to ptr, advance ptr, return new ptr
 * -------------------------------------------------------------------------- */
static char *append(char *ptr, const char *s)
{
	StringLength len = fun_string_length((String)s);
	fun_string_copy((String)s, ptr);
	return ptr + len;
}

/* --------------------------------------------------------------------------
 * Write script buffer to file
 * -------------------------------------------------------------------------- */
static BuildGenerationResult write_script(const char *script_path,
                                          const char *content)
{
	BuildGenerationResult result;
	result.script_path = (String)script_path;

	StringLength total_len = fun_string_length((String)content);
	MemoryResult mem_result = fun_memory_allocate(total_len);
	if (fun_error_is_error(mem_result.error)) {
		result.status = BUILD_GENERATED_ERROR;
		result.error_message = (String)"Failed to allocate memory";
		return result;
	}
	fun_string_copy((String)content, (char *)mem_result.value);

	AsyncResult write_result = fun_write_memory_to_file(
		(Write){ .file_path = (String)script_path,
			     .input = mem_result.value,
			     .bytes_to_write = total_len });
	fun_async_await(&write_result, -1);

	voidResult free_result = fun_memory_free(&mem_result.value);
	(void)free_result;

	if (write_result.status == ASYNC_COMPLETED) {
		result.status = BUILD_GENERATED_SUCCESS;
		result.error_message = (String)"";
	} else {
		result.status = BUILD_GENERATED_ERROR;
		result.error_message = (String)"Failed to write file";
	}

	return result;
}

/* --------------------------------------------------------------------------
 * Generate Windows build script
 * -------------------------------------------------------------------------- */
BuildGenerationResult build_generate_windows(SourceScanResult scan_result)
{
	static char script_buffer[8192];
	static char sources_buffer[MAX_SOURCES_LENGTH];
	char name[64];

	/* Read project name from fun.ini, fall back to "app" */
	if (!fun_ini_read_name(name, sizeof(name))) {
		fun_string_copy((String)"app", name);
	}

	build_sources_string(scan_result, sources_buffer, sizeof(sources_buffer));

	char *ptr = script_buffer;

	ptr = append(ptr,
	    "@ECHO OFF\r\n"
	    "REM Build script generated by fun build tool\r\n"
	    "REM Platform: windows-amd64\r\n"
	    "\r\n"
	    "if not exist build mkdir build\r\n"
	    "\r\n"
	    "REM Compile with GCC\r\n"
	    "gcc --std=c17 -Os -nostdlib -fno-builtin -fno-exceptions"
	    " -fno-unwind-tables -mno-stack-arg-probe -e main -mconsole"
	    " -I . -I vendor/fundamental/include"
	    " vendor/fundamental/src/startup/startup.c"
	    " vendor/fundamental/arch/startup/windows-amd64/windows.c ");

	/* Project source files */
	ptr = append(ptr, sources_buffer);

	/* Comprehensive fundamental vendor modules */
	ptr = append(ptr,
	    "vendor/fundamental/src/platform/platform.c"
	    " vendor/fundamental/arch/platform/windows-amd64/platform.c"
	    " vendor/fundamental/src/async/async.c"
	    " vendor/fundamental/src/process/process.c"
	    " vendor/fundamental/arch/process/windows-amd64/process.c"
	    " vendor/fundamental/arch/file/windows-amd64/fileRead.c"
	    " vendor/fundamental/arch/file/windows-amd64/fileReadMmap.c"
	    " vendor/fundamental/arch/file/windows-amd64/fileReadRing.c"
	    " vendor/fundamental/arch/file/windows-amd64/fileWrite.c"
	    " vendor/fundamental/arch/file/windows-amd64/fileWriteMmap.c"
	    " vendor/fundamental/arch/file/windows-amd64/fileWriteRing.c"
	    " vendor/fundamental/src/console/console.c"
	    " vendor/fundamental/src/string/stringConversion.c"
	    " vendor/fundamental/src/string/stringOperations.c"
	    " vendor/fundamental/src/string/stringTemplate.c"
	    " vendor/fundamental/src/string/stringValidation.c"
	    " vendor/fundamental/src/array/array.c"
	    " vendor/fundamental/arch/console/windows-amd64/console.c"
	    " vendor/fundamental/arch/memory/windows-amd64/memory.c"
	    " vendor/fundamental/src/filesystem/directory.c"
	    " vendor/fundamental/src/filesystem/file_exists.c"
	    " vendor/fundamental/src/filesystem/path.c"
	    " vendor/fundamental/arch/filesystem/windows-amd64/directory.c"
	    " vendor/fundamental/arch/filesystem/windows-amd64/file_exists.c"
	    " vendor/fundamental/arch/filesystem/windows-amd64/path.c"
	    " -lkernel32 -o build/");

	/* Binary name: build/<name>-windows-amd64.exe */
	ptr = append(ptr, name);
	ptr = append(ptr, "-windows-amd64.exe\r\n\r\n");
	ptr = append(ptr, "strip --strip-unneeded build/");
	ptr = append(ptr, name);
	ptr = append(ptr, "-windows-amd64.exe\r\n\r\n");
	ptr = append(ptr, "ECHO Build complete: build/");
	ptr = append(ptr, name);
	ptr = append(ptr, "-windows-amd64.exe\r\n");
	*ptr = '\0';

	return write_script("build-windows-amd64.bat", script_buffer);
}

/* --------------------------------------------------------------------------
 * Generate Linux build script
 * -------------------------------------------------------------------------- */
BuildGenerationResult build_generate_linux(SourceScanResult scan_result)
{
	static char script_buffer[8192];
	static char sources_buffer[MAX_SOURCES_LENGTH];
	char name[64];

	if (!fun_ini_read_name(name, sizeof(name))) {
		fun_string_copy((String)"app", name);
	}

	build_sources_string(scan_result, sources_buffer, sizeof(sources_buffer));

	char *ptr = script_buffer;

	ptr = append(ptr,
	    "#!/bin/bash\n"
	    "# Build script generated by fun build tool\n"
	    "# Platform: linux-amd64\n"
	    "\n"
	    "mkdir -p build\n"
	    "\n"
	    "# Compile with GCC\n"
	    "gcc --std=c17 -Os -nostdlib -fno-builtin -fno-exceptions"
	    " -fno-unwind-tables -e main"
	    " -I . -I vendor/fundamental/include"
	    " vendor/fundamental/src/startup/startup.c"
	    " vendor/fundamental/arch/startup/linux-amd64/linux.c ");

	ptr = append(ptr, sources_buffer);

	ptr = append(ptr,
	    "vendor/fundamental/src/platform/platform.c"
	    " vendor/fundamental/arch/platform/linux-amd64/platform.c"
	    " vendor/fundamental/src/async/async.c"
	    " vendor/fundamental/src/process/process.c"
	    " vendor/fundamental/arch/process/linux-amd64/process.c"
	    " vendor/fundamental/arch/file/linux-amd64/fileRead.c"
	    " vendor/fundamental/arch/file/linux-amd64/fileReadMmap.c"
	    " vendor/fundamental/arch/file/linux-amd64/fileReadRing.c"
	    " vendor/fundamental/arch/file/linux-amd64/fileWrite.c"
	    " vendor/fundamental/arch/file/linux-amd64/fileWriteMmap.c"
	    " vendor/fundamental/arch/file/linux-amd64/fileWriteRing.c"
	    " vendor/fundamental/src/console/console.c"
	    " vendor/fundamental/src/string/stringConversion.c"
	    " vendor/fundamental/src/string/stringOperations.c"
	    " vendor/fundamental/src/string/stringTemplate.c"
	    " vendor/fundamental/src/string/stringValidation.c"
	    " vendor/fundamental/src/array/array.c"
	    " vendor/fundamental/arch/console/linux-amd64/console.c"
	    " vendor/fundamental/arch/memory/linux-amd64/memory.c"
	    " vendor/fundamental/src/filesystem/directory.c"
	    " vendor/fundamental/src/filesystem/file_exists.c"
	    " vendor/fundamental/src/filesystem/path.c"
	    " vendor/fundamental/arch/filesystem/linux-amd64/directory.c"
	    " vendor/fundamental/arch/filesystem/linux-amd64/file_exists.c"
	    " vendor/fundamental/arch/filesystem/linux-amd64/path.c"
	    " -o build/");

	ptr = append(ptr, name);
	ptr = append(ptr, "-linux-amd64\n\n");
	ptr = append(ptr, "strip --strip-unneeded build/");
	ptr = append(ptr, name);
	ptr = append(ptr, "-linux-amd64\n\n");
	ptr = append(ptr, "echo Build complete: build/");
	ptr = append(ptr, name);
	ptr = append(ptr, "-linux-amd64\n");
	*ptr = '\0';

	return write_script("build-linux-amd64.sh", script_buffer);
}

/* --------------------------------------------------------------------------
 * Generate build script for current platform
 * -------------------------------------------------------------------------- */
BuildGenerationResult build_generate_current(void)
{
	Platform platform = platform_get();
	SourceScanResult scan_result = build_scan_sources();

	if (scan_result.count == 0) {
		BuildGenerationResult error_result;
		error_result.status = BUILD_GENERATED_ERROR;
		error_result.error_message = (String)"No source files found";
		return error_result;
	}

	if (platform.os == PLATFORM_OS_WINDOWS) {
		return build_generate_windows(scan_result);
	} else {
		return build_generate_linux(scan_result);
	}
}
