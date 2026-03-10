#include "fileRead.h"

static DWORD get_allocation_granularity()
{
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	return si.dwAllocationGranularity;
}

AsyncStatus poll_mmap(AsyncResult *result)
{
	MMapState *state = (MMapState *)result->state;
	AsyncStatus final_status = ASYNC_COMPLETED;

	if (!state->file_handle) {
		// Step 1: Open file
		wchar_t wide_path_buffer[MAX_PATH]; // Stack allocation
		int chars_converted =
			MultiByteToWideChar(CP_UTF8, 0, state->parameters.file_path, -1,
								wide_path_buffer, MAX_PATH);
		if (chars_converted == 0) {
			result->error =
				(ErrorResult){ .code = GetLastError(),
							   .message = "MultiByteToWideChar error" };
			// Set status and jump to cleanup (though handles aren't open yet)
			final_status = ASYNC_ERROR;
			goto cleanup;
		}

		state->file_handle = CreateFileW(wide_path_buffer, GENERIC_READ,
										 FILE_SHARE_READ, NULL, OPEN_EXISTING,
										 FILE_ATTRIBUTE_NORMAL, NULL);
		if (state->file_handle == INVALID_HANDLE_VALUE) {
			result->error =
				(ErrorResult){ .code = GetLastError(),
							   .message =
								   "CreateFileW error, INVALID_HANDLE_VALUE" };
			final_status = ASYNC_ERROR;
			goto cleanup; // Jump to cleanup
		}
		return ASYNC_PENDING; // Still pending after this step
	}

	if (!state->mapping_handle) {
		// Step 2: Create file mapping
		state->mapping_handle = CreateFileMappingW(state->file_handle, NULL,
												   PAGE_READONLY, 0, 0, NULL);

		if (!state->mapping_handle) {
			result->error = (ErrorResult){
				.code = GetLastError(),
				.message = "CreateFileMappingW error, mapping_handle null"
			};
			// Set status and jump to cleanup
			final_status = ASYNC_ERROR;
			goto cleanup;
		}
		return ASYNC_PENDING; // Still pending after this step
	}

	if (!state->mapped_view) {
		// Step 3: Calculate aligned offset
		DWORD granularity = get_allocation_granularity();
		state->adjusted_offset =
			state->parameters.offset / granularity * granularity;
		// Step 4: Map view of file
		DWORD offset_high = (DWORD)(state->adjusted_offset >> 32);
		DWORD offset_low = (DWORD)(state->adjusted_offset & 0xFFFFFFFF);
		SIZE_T view_size = state->parameters.bytes_to_read +
						   (state->parameters.offset - state->adjusted_offset);

		state->mapped_view = MapViewOfFile(state->mapping_handle, FILE_MAP_READ,
										   offset_high, offset_low, view_size);

		if (!state->mapped_view) {
			result->error =
				(ErrorResult){ .code = GetLastError(),
							   .message =
								   "MapViewOfFile error, mapped_view null" };
			// Set status and jump to cleanup
			final_status = ASYNC_ERROR;
			goto cleanup;
		}
		return ASYNC_PENDING; // Still pending after this step
	}

	// Step 5: Copy data and cleanup
	BYTE *data_ptr = (BYTE *)state->mapped_view +
					 (state->parameters.offset - state->adjusted_offset);
	voidResult copy_result = fun_memory_copy(data_ptr, state->parameters.output,
											 state->parameters.bytes_to_read);
	if (fun_error_is_error(copy_result.error)) {
		result->error = copy_result.error;
		// Set status to error
		final_status = ASYNC_ERROR;
		// Jump to the common cleanup block
		goto cleanup;
	}

cleanup:
	// Common cleanup logic
	if (state) { // Check if state was allocated
		if (state->mapped_view) {
			UnmapViewOfFile(state->mapped_view);
		}
		if (state->mapping_handle) {
			CloseHandle(state->mapping_handle);
		}
		if (state->file_handle &&
			state->file_handle !=
				INVALID_HANDLE_VALUE) { // Check handle validity
			CloseHandle(state->file_handle);
		}
		// explicit cast to avoid incompatible-pointer-types
		fun_memory_free((Memory *)&state); // Free the state memory
	}
	// Return the determined status
	return final_status;
}
