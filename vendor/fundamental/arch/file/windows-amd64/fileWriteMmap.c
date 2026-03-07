#include "fileWrite.h"

AsyncStatus poll_mmap_write(AsyncResult *result)
{
	MMapWriteState *state = (MMapWriteState *)result->state;
	AsyncStatus final_status = ASYNC_COMPLETED;

	// Step 1: Open/Create File
	if (state->file_handle == INVALID_HANDLE_VALUE) {
		// Convert UTF-8 path to wide string
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
										 FILE_SHARE_READ, NULL,
										 OPEN_ALWAYS, // Create if doesn't exist
										 FILE_ATTRIBUTE_NORMAL, NULL);

		if (state->file_handle == INVALID_HANDLE_VALUE) {
			result->error =
				(ErrorResult){ .code = GetLastError(),
							   .message = "Failed to open/create file" };
			return ASYNC_ERROR;
		}
		return ASYNC_PENDING;
	}

	// Step 2: Handle File Extension
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

	// Step 3: Create File Mapping
	if (!state->mapping_handle) {
		state->mapping_handle = CreateFileMappingW(state->file_handle, NULL,
												   PAGE_READWRITE, 0,
												   0, // Use entire file size
												   NULL // Unnamed mapping
		);

		if (!state->mapping_handle) {
			result->error =
				(ErrorResult){ .code = GetLastError(),
							   .message = "Failed to create file mapping" };
			final_status = ASYNC_ERROR;
			goto cleanup;
		}
		return ASYNC_PENDING;
	}

	// Step 4: Map View and Write Data
	if (!state->mapped_view) {
		// Calculate aligned offset
		SYSTEM_INFO sys_info;
		GetSystemInfo(&sys_info);
		uint64_t granularity = sys_info.dwAllocationGranularity;
		state->adjusted_offset =
			(state->parameters.offset / granularity) * granularity;

		// Calculate view size including alignment padding
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

		// Copy data to mapped memory
		uint64_t actual_offset =
			state->parameters.offset - state->adjusted_offset;
		void *write_location = (char *)state->mapped_view + actual_offset;
		memcpy(write_location, state->parameters.input,
			   state->parameters.bytes_to_write);

		// Force synchronization to disk
		if (!FlushViewOfFile(state->mapped_view, view_size)) {
			result->error =
				(ErrorResult){ .code = GetLastError(),
							   .message = "Failed to flush data to disk" };
			final_status = ASYNC_ERROR;
			goto cleanup;
		}

		// Also flush file buffers
		if (!FlushFileBuffers(state->file_handle)) {
			result->error =
				(ErrorResult){ .code = GetLastError(),
							   .message =
								   "Failed to flush file buffers to disk" };
			final_status = ASYNC_ERROR;
			goto cleanup;
		}

		final_status = ASYNC_COMPLETED;
	}

cleanup:
	// Cleanup resources in reverse order
	if (state->mapped_view) {
		UnmapViewOfFile(state->mapped_view);
	}
	if (state->mapping_handle) {
		CloseHandle(state->mapping_handle);
	}
	if (state->file_handle != INVALID_HANDLE_VALUE) {
		CloseHandle(state->file_handle);
	}

	// Free state memory
	fun_memory_free((void **)&state);

	return final_status;
}
