#include "fileWrite.h"
#include "fileAdaptive.h"

AsyncStatus poll_mmap_write(AsyncResult *result)
{
	MMapWriteState *state = (MMapWriteState *)result->state;
	FileAdaptiveState *adaptive = state->parameters.adaptive;
	uint64_t bytes = state->parameters.bytes_to_write;
	AsyncStatus final_status = ASYNC_COMPLETED;

	if (state->file_handle == INVALID_HANDLE_VALUE) {
		wchar_t wide_path_buffer[MAX_PATH];
		int chars_converted =
			MultiByteToWideChar(CP_UTF8, 0, state->parameters.file_path, -1,
								wide_path_buffer, MAX_PATH);
		if (chars_converted == 0) {
			result->error =
				(ErrorResult){ .code = GetLastError(),
							   .message = "Path conversion failed" };
			return ASYNC_ERROR;
		}

		state->file_handle = CreateFileW(wide_path_buffer,
										 GENERIC_READ | GENERIC_WRITE,
										 FILE_SHARE_READ, NULL, OPEN_ALWAYS,
										 FILE_ATTRIBUTE_NORMAL, NULL);
		if (state->file_handle == INVALID_HANDLE_VALUE) {
			result->error =
				(ErrorResult){ .code = GetLastError(),
							   .message = "Failed to open/create file" };
			return ASYNC_ERROR;
		}
		return ASYNC_PENDING;
	}

	if (!state->file_extended) {
		LARGE_INTEGER file_size;
		if (!GetFileSizeEx(state->file_handle, &file_size)) {
			result->error =
				(ErrorResult){ .code = GetLastError(),
							   .message = "Failed to get file size" };
			final_status = ASYNC_ERROR;
			goto cleanup;
		}

		state->original_file_size = file_size.QuadPart;
		uint64_t required_size =
			state->parameters.offset + state->parameters.bytes_to_write;

		if (required_size > state->original_file_size) {
			LARGE_INTEGER new_pos;
			new_pos.QuadPart = required_size;
			if (!SetFilePointerEx(state->file_handle, new_pos, NULL,
								  FILE_BEGIN) ||
				!SetEndOfFile(state->file_handle)) {
				result->error =
					(ErrorResult){ .code = GetLastError(),
								   .message = "Failed to extend file" };
				final_status = ASYNC_ERROR;
				goto cleanup;
			}
		}
		state->file_extended = true;
		return ASYNC_PENDING;
	}

	if (!state->mapping_handle) {
		state->mapping_handle = CreateFileMappingW(state->file_handle, NULL,
												   PAGE_READWRITE, 0, 0, NULL);
		if (!state->mapping_handle) {
			result->error =
				(ErrorResult){ .code = GetLastError(),
							   .message = "Failed to create file mapping" };
			final_status = ASYNC_ERROR;
			goto cleanup;
		}
		return ASYNC_PENDING;
	}

	if (!state->mapped_view) {
		SYSTEM_INFO sys_info;
		GetSystemInfo(&sys_info);
		uint64_t granularity = sys_info.dwAllocationGranularity;
		state->adjusted_offset =
			(state->parameters.offset / granularity) * granularity;
		uint64_t view_size =
			state->parameters.bytes_to_write +
			(state->parameters.offset - state->adjusted_offset);
		DWORD offset_high = (DWORD)(state->adjusted_offset >> 32);
		DWORD offset_low = (DWORD)(state->adjusted_offset & 0xFFFFFFFF);

		state->mapped_view = MapViewOfFile(state->mapping_handle,
										   FILE_MAP_WRITE, offset_high,
										   offset_low, view_size);
		if (!state->mapped_view) {
			result->error =
				(ErrorResult){ .code = GetLastError(),
							   .message = "Failed to map view of file" };
			final_status = ASYNC_ERROR;
			goto cleanup;
		}

		uint64_t actual_offset =
			state->parameters.offset - state->adjusted_offset;
		void *write_location = (char *)state->mapped_view + actual_offset;
		fun_memory_copy(state->parameters.input, write_location,
						state->parameters.bytes_to_write);

		uint64_t view_size_flush =
			state->parameters.bytes_to_write +
			(state->parameters.offset - state->adjusted_offset);
		if (!FlushViewOfFile(state->mapped_view, view_size_flush) ||
			!FlushFileBuffers(state->file_handle)) {
			result->error = (ErrorResult){ .code = GetLastError(),
										   .message = "Failed to flush data" };
			final_status = ASYNC_ERROR;
		}
	}

cleanup:
	if (state->mapped_view)
		UnmapViewOfFile(state->mapped_view);
	if (state->mapping_handle)
		CloseHandle(state->mapping_handle);
	if (state->file_handle != INVALID_HANDLE_VALUE)
		CloseHandle(state->file_handle);
	void *mem = state;
	fun_memory_free(&mem);
	if (final_status == ASYNC_COMPLETED)
		file_adaptive_update(adaptive, bytes);
	return final_status;
}
