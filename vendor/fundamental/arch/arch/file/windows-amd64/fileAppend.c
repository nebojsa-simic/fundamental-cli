#include "fundamental/file/file.h"
#include "fundamental/memory/memory.h"
#include "fileAdaptive.h"

#include <windows.h>

#define WINAPI_FAMILY WINAPI_FAMILY_DESKTOP_APP
#undef NTDDI_VERSION
#define NTDDI_VERSION NTDDI_WIN10_NI

#include "ioringcompat.h"
#include <ioringapi.h>

// ------------------------------------------------------------------
// mmap-based append
// ------------------------------------------------------------------

typedef enum {
	MMAP_APPEND_OPEN,
	MMAP_APPEND_EXTEND,
	MMAP_APPEND_MAP,
} MMapAppendStep;

typedef struct {
	Append parameters;
	HANDLE file_handle;
	HANDLE mapping_handle;
	LPVOID mapped_view;
	uint64_t file_size;
	MMapAppendStep step;
} MMapAppendState;

static AsyncStatus poll_mmap_append(AsyncResult *result)
{
	MMapAppendState *state = (MMapAppendState *)result->state;
	FileAdaptiveState *adaptive = state->parameters.adaptive;
	uint64_t bytes = state->parameters.bytes_to_append;
	AsyncStatus final_status = ASYNC_COMPLETED;

	if (state->step == MMAP_APPEND_OPEN) {
		wchar_t wide_path[MAX_PATH];
		if (MultiByteToWideChar(CP_UTF8, 0, state->parameters.file_path, -1,
								wide_path, MAX_PATH) == 0) {
			result->error =
				(ErrorResult){ .code = GetLastError(),
							   .message = "Path conversion failed" };
			final_status = ASYNC_ERROR;
			goto cleanup;
		}

		state->file_handle = CreateFileW(wide_path,
										 GENERIC_READ | GENERIC_WRITE,
										 FILE_SHARE_READ, NULL, OPEN_ALWAYS,
										 FILE_ATTRIBUTE_NORMAL, NULL);
		if (state->file_handle == INVALID_HANDLE_VALUE) {
			result->error = (ErrorResult){ .code = GetLastError(),
										   .message = "Failed to open file" };
			final_status = ASYNC_ERROR;
			goto cleanup;
		}

		LARGE_INTEGER size;
		if (!GetFileSizeEx(state->file_handle, &size)) {
			result->error =
				(ErrorResult){ .code = GetLastError(),
							   .message = "Failed to get file size" };
			final_status = ASYNC_ERROR;
			goto cleanup;
		}
		state->file_size = (uint64_t)size.QuadPart;
		state->step = MMAP_APPEND_EXTEND;
		return ASYNC_PENDING;
	}

	if (state->step == MMAP_APPEND_EXTEND) {
		uint64_t new_size =
			state->file_size + state->parameters.bytes_to_append;
		LARGE_INTEGER new_pos;
		new_pos.QuadPart = new_size;
		if (!SetFilePointerEx(state->file_handle, new_pos, NULL, FILE_BEGIN) ||
			!SetEndOfFile(state->file_handle)) {
			result->error = (ErrorResult){ .code = GetLastError(),
										   .message = "Failed to extend file" };
			final_status = ASYNC_ERROR;
			goto cleanup;
		}

		state->mapping_handle = CreateFileMappingW(state->file_handle, NULL,
												   PAGE_READWRITE, 0, 0, NULL);
		if (!state->mapping_handle) {
			result->error =
				(ErrorResult){ .code = GetLastError(),
							   .message = "Failed to create file mapping" };
			final_status = ASYNC_ERROR;
			goto cleanup;
		}
		state->step = MMAP_APPEND_MAP;
		return ASYNC_PENDING;
	}

	// MMAP_APPEND_MAP: map the appended region and copy
	SYSTEM_INFO sys_info;
	GetSystemInfo(&sys_info);
	uint64_t granularity = sys_info.dwAllocationGranularity;
	uint64_t aligned_offset = (state->file_size / granularity) * granularity;
	uint64_t view_size =
		state->parameters.bytes_to_append + (state->file_size - aligned_offset);

	DWORD offset_high = (DWORD)(aligned_offset >> 32);
	DWORD offset_low = (DWORD)(aligned_offset & 0xFFFFFFFF);

	state->mapped_view = MapViewOfFile(state->mapping_handle, FILE_MAP_WRITE,
									   offset_high, offset_low, view_size);
	if (!state->mapped_view) {
		result->error =
			(ErrorResult){ .code = GetLastError(),
						   .message = "Failed to map view for append" };
		final_status = ASYNC_ERROR;
		goto cleanup;
	}

	void *write_ptr =
		(char *)state->mapped_view + (state->file_size - aligned_offset);
	fun_memory_copy(state->parameters.input, write_ptr,
					state->parameters.bytes_to_append);

	if (!FlushViewOfFile(state->mapped_view, view_size) ||
		!FlushFileBuffers(state->file_handle)) {
		result->error =
			(ErrorResult){ .code = GetLastError(), .message = "Flush failed" };
		final_status = ASYNC_ERROR;
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

// ------------------------------------------------------------------
// ring-based append
// ------------------------------------------------------------------

typedef struct {
	HIORING io_ring;
	IORING_INFO info;
} IoRingAppendContext;

typedef struct {
	Append parameters;
	IoRingAppendContext *global_context;
	HANDLE file_handle;
	uint64_t request_id;
	AsyncStatus async_status;
} RingAppendState;

static IoRingAppendContext global_append_context = { 0 };
static uint64_t next_append_request_id = 0;

static AsyncStatus poll_ring_append(AsyncResult *result)
{
	RingAppendState *state = result->state;
	IORING_CQE cqe;

	if (state->async_status != ASYNC_PENDING) {
		AsyncStatus status = state->async_status;
		FileAdaptiveState *adaptive = state->parameters.adaptive;
		uint64_t bytes = state->parameters.bytes_to_append;
		if (state->file_handle != INVALID_HANDLE_VALUE)
			CloseHandle(state->file_handle);
		void *mem = state;
		fun_memory_free(&mem);
		if (status == ASYNC_COMPLETED)
			file_adaptive_update(adaptive, bytes);
		return status;
	}

	if (SUCCEEDED(PopIoRingCompletion(state->global_context->io_ring, &cqe))) {
		AsyncStatus status = cqe.ResultCode >= 0 ? ASYNC_COMPLETED :
												   ASYNC_ERROR;
		state->async_status = status;
		if (cqe.UserData == state->request_id) {
			FileAdaptiveState *adaptive = state->parameters.adaptive;
			uint64_t bytes = state->parameters.bytes_to_append;
			if (state->file_handle != INVALID_HANDLE_VALUE)
				CloseHandle(state->file_handle);
			void *mem = state;
			fun_memory_free(&mem);
			if (status == ASYNC_COMPLETED)
				file_adaptive_update(adaptive, bytes);
			return status;
		}
	}
	return ASYNC_PENDING;
}

static AsyncResult create_ring_append(Append parameters)
{
	if (!global_append_context.io_ring) {
		IORING_CREATE_FLAGS flags = {
			.Advisory = IORING_CREATE_ADVISORY_FLAGS_NONE,
			.Required = IORING_CREATE_REQUIRED_FLAGS_NONE
		};
		HRESULT hr = CreateIoRing(IORING_VERSION_1, flags, 0, 0,
								  &global_append_context.io_ring);
		if (FAILED(hr))
			return (AsyncResult){
				.status = ASYNC_ERROR,
				.error = { .code = hr, .message = "CreateIoRing failed" }
			};
		GetIoRingInfo(global_append_context.io_ring,
					  &global_append_context.info);
	}

	MemoryResult mem_result = fun_memory_allocate(sizeof(RingAppendState));
	if (fun_error_is_error(mem_result.error))
		return (AsyncResult){ .status = ASYNC_ERROR,
							  .error = mem_result.error };

	HANDLE file = INVALID_HANDLE_VALUE;
	RingAppendState *state = (RingAppendState *)mem_result.value;
	state->global_context = &global_append_context;
	state->request_id = next_append_request_id++;
	state->async_status = ASYNC_PENDING;
	state->parameters = parameters;

	// Open with FILE_APPEND_DATA so the OS handles the offset atomically
	file = CreateFile(parameters.file_path, FILE_APPEND_DATA, FILE_SHARE_READ,
					  NULL, OPEN_ALWAYS, FILE_FLAG_OVERLAPPED, NULL);
	if (file == INVALID_HANDLE_VALUE)
		goto cleanup;

	state->file_handle = file;

	IORING_HANDLE_REF file_ref = IoRingHandleRefFromHandle(file);
	IORING_BUFFER_REF buffer_ref = IoRingBufferRefFromPointer(parameters.input);

	// offset = -1 means append (FILE_USE_FILE_POINTER_POSITION)
	HRESULT hr = BuildIoRingWriteFile(state->global_context->io_ring, file_ref,
									  buffer_ref, parameters.bytes_to_append,
									  (UINT64)-1, FILE_WRITE_FLAGS_NONE,
									  state->request_id, IOSQE_FLAGS_NONE);
	if (FAILED(hr))
		goto cleanup;

	UINT32 submitted;
	hr = SubmitIoRing(state->global_context->io_ring, 0, 1, &submitted);
	if (FAILED(hr))
		goto cleanup;

	return (AsyncResult){ .state = state,
						  .poll = poll_ring_append,
						  .status = ASYNC_PENDING };

cleanup:
	if (state) {
		void *mem = state;
		fun_memory_free(&mem);
	}
	if (file != INVALID_HANDLE_VALUE)
		CloseHandle(file);
	return (AsyncResult){ .status = ASYNC_ERROR,
						  .error = { .code = 1,
									 .message = "Ring append setup failed" } };
}

// ------------------------------------------------------------------
// dispatch
// ------------------------------------------------------------------

AsyncResult fun_append_memory_to_file(Append parameters)
{
	if (!parameters.file_path || !parameters.input)
		return (AsyncResult){ .status = ASYNC_ERROR,
							  .error = ERROR_RESULT_NULL_POINTER };

	if (parameters.bytes_to_append == 0)
		return (AsyncResult){ .status = ASYNC_COMPLETED,
							  .error = ERROR_RESULT_NO_ERROR };

	FileMode mode = parameters.mode;
	if (mode == FILE_MODE_AUTO)
		mode = file_adaptive_choose(parameters.adaptive);

	if (mode == FILE_MODE_RING_BASED)
		return create_ring_append(parameters);

	MemoryResult mem_result = fun_memory_allocate(sizeof(MMapAppendState));
	if (fun_error_is_error(mem_result.error))
		return (AsyncResult){ .status = ASYNC_ERROR,
							  .error = mem_result.error };

	MMapAppendState *state = (MMapAppendState *)mem_result.value;
	*state = (MMapAppendState){ .parameters = parameters,
								.file_handle = INVALID_HANDLE_VALUE,
								.step = MMAP_APPEND_OPEN };

	return (AsyncResult){ .state = state,
						  .poll = poll_mmap_append,
						  .status = ASYNC_PENDING };
}
