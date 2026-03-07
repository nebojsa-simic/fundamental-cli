#pragma once

#include "fileRead.h"

#include <windows.h>

#define WINAPI_FAMILY WINAPI_FAMILY_DESKTOP_APP
#define NTDDI_VERSION NTDDI_WIN10_NI

#include <ioringapi.h>

typedef struct {
	HIORING io_ring;
	IORING_CREATE_FLAGS flags;
	IORING_INFO info;
} IoRingContext;

typedef struct {
	IoRingContext *global_context;
	HANDLE file_handle;
	uint64_t request_id;
	AsyncStatus async_status;
} RingReadState;

// Initialize I/O Ring with default parameters
static inline ErrorResult initialize_io_ring(IoRingContext *ctx)
{
	IORING_CREATE_FLAGS flags =
		(IORING_CREATE_FLAGS){ .Advisory = IORING_CREATE_ADVISORY_FLAGS_NONE,
							   .Required = IORING_CREATE_REQUIRED_FLAGS_NONE };

	HRESULT hr = CreateIoRing(IORING_VERSION_1, flags, 0, 0, &ctx->io_ring);
	if (FAILED(hr)) {
		return (ErrorResult){ .code = hr };
	}

	GetIoRingInfo(ctx->io_ring, &ctx->info);
	return ERROR_RESULT_NO_ERROR;
}

static inline AsyncStatus poll_io_ring(AsyncResult *result)
{
	RingReadState *state = result->state;
	IORING_CQE cqe;

	if (state->async_status != ASYNC_PENDING) {
		AsyncStatus status = state->async_status;
		if (state->file_handle != INVALID_HANDLE_VALUE) {
			CloseHandle(state->file_handle);
			fun_memory_free(&state);
		}
		return status;
	} else if (SUCCEEDED(
				   PopIoRingCompletion(state->global_context->io_ring, &cqe))) {
		AsyncStatus status = cqe.ResultCode >= 0 ? ASYNC_COMPLETED :
												   ASYNC_ERROR;
		state->async_status = status;
		if (cqe.UserData == state->request_id) {
			if (state->file_handle != INVALID_HANDLE_VALUE) {
				CloseHandle(state->file_handle);
				fun_memory_free(&state);
			}
			return status;
		}
	}
	return ASYNC_PENDING;
}

AsyncResult fun_read_file_in_memory(Read parameters)
{
	static IoRingContext global_context = { 0 };
	static uint64_t request_id = 0;

	MemoryResult memory_allocation_result =
		fun_memory_allocate(sizeof(RingReadState));
	if (fun_error_is_error(memory_allocation_result.error)) {
		return (AsyncResult){ .status = ASYNC_ERROR,
							  .error = memory_allocation_result.error };
	}

	HANDLE file = INVALID_HANDLE_VALUE;
	RingReadState *state = (RingReadState *)memory_allocation_result.value;

	state->global_context = &global_context;
	state->request_id = request_id++;

	if (!state->global_context->io_ring) {
		ErrorResult initialization_result =
			initialize_io_ring(state->global_context);
		if (fun_error_is_error(initialization_result)) {
			goto cleanup;
		}
	}

	file = CreateFile(parameters.file_path, GENERIC_READ, FILE_SHARE_READ, NULL,
					  OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);

	if (file == INVALID_HANDLE_VALUE) {
		goto cleanup;
	}

	state->file_handle = file;

	// Register file handle and buffer
	IORING_HANDLE_REF file_ref = IoRingHandleRefFromHandle(file);
	IORING_BUFFER_REF buffer_ref =
		IoRingBufferRefFromPointer(parameters.output);

	// Build read operation
	HRESULT hr = BuildIoRingReadFile(state->global_context->io_ring, file_ref,
									 buffer_ref, parameters.bytes_to_read,
									 parameters.offset, state->request_id,
									 IOSQE_FLAGS_NONE);

	if (FAILED(hr)) {
		goto cleanup;
	}

	// Submit operations
	ULONG submitted;
	hr = SubmitIoRing(state->global_context->io_ring,
					  0, // No wait
					  1, // One operation
					  &submitted);

	if (FAILED(hr)) {
		goto cleanup;
	}

	return (AsyncResult){ .state = state,
						  .poll = poll_io_ring,
						  .status = ASYNC_PENDING };

cleanup:
	if (state) {
		fun_memory_free(&state);
	}
	if (file != INVALID_HANDLE_VALUE) {
		CloseHandle(file);
	}
	return (AsyncResult){ .status = ASYNC_ERROR,
						  .error = { .code = hr, .message = 'Read error' } };
}
