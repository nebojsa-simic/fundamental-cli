#include "fileWrite.h"
#include "fileAdaptive.h"

#include <windows.h>

#define WINAPI_FAMILY WINAPI_FAMILY_DESKTOP_APP
#undef NTDDI_VERSION
#define NTDDI_VERSION NTDDI_WIN10_NI

#include "ioringcompat.h"
#include <ioringapi.h>

typedef struct {
	HIORING io_ring;
	IORING_CREATE_FLAGS flags;
	IORING_INFO info;
} IoRingWriteContext;

typedef struct {
	Write parameters;
	IoRingWriteContext *global_context;
	HANDLE file_handle;
	uint64_t request_id;
	AsyncStatus async_status;
} RingWriteState;

static IoRingWriteContext global_write_context = { 0 };
static uint64_t next_write_request_id = 0;

static inline ErrorResult initialize_write_io_ring(IoRingWriteContext *ctx)
{
	IORING_CREATE_FLAGS flags =
		(IORING_CREATE_FLAGS){ .Advisory = IORING_CREATE_ADVISORY_FLAGS_NONE,
							   .Required = IORING_CREATE_REQUIRED_FLAGS_NONE };

	HRESULT hr = CreateIoRing(IORING_VERSION_1, flags, 0, 0, &ctx->io_ring);
	if (FAILED(hr))
		return (ErrorResult){ .code = hr };

	GetIoRingInfo(ctx->io_ring, &ctx->info);
	return ERROR_RESULT_NO_ERROR;
}

static AsyncStatus poll_ring_write(AsyncResult *result)
{
	RingWriteState *state = result->state;
	IORING_CQE cqe;

	if (state->async_status != ASYNC_PENDING) {
		AsyncStatus status = state->async_status;
		FileAdaptiveState *adaptive = state->parameters.adaptive;
		uint64_t bytes = state->parameters.bytes_to_write;
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
			uint64_t bytes = state->parameters.bytes_to_write;
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

AsyncResult create_ring_write(Write parameters)
{
	MemoryResult mem_result = fun_memory_allocate(sizeof(RingWriteState));
	if (fun_error_is_error(mem_result.error))
		return (AsyncResult){ .status = ASYNC_ERROR,
							  .error = mem_result.error };

	HANDLE file = INVALID_HANDLE_VALUE;
	RingWriteState *state = (RingWriteState *)mem_result.value;
	state->global_context = &global_write_context;
	state->request_id = next_write_request_id++;
	state->async_status = ASYNC_PENDING;
	state->parameters = parameters;

	if (!state->global_context->io_ring) {
		ErrorResult init = initialize_write_io_ring(state->global_context);
		if (fun_error_is_error(init))
			goto cleanup;
	}

	file = CreateFile(parameters.file_path, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS,
					  FILE_FLAG_OVERLAPPED, NULL);
	if (file == INVALID_HANDLE_VALUE)
		goto cleanup;

	state->file_handle = file;

	IORING_HANDLE_REF file_ref = IoRingHandleRefFromHandle(file);
	IORING_BUFFER_REF buffer_ref = IoRingBufferRefFromPointer(parameters.input);

	HRESULT hr = BuildIoRingWriteFile(state->global_context->io_ring, file_ref,
									  buffer_ref, parameters.bytes_to_write,
									  parameters.offset, FILE_WRITE_FLAGS_NONE,
									  state->request_id, IOSQE_FLAGS_NONE);
	if (FAILED(hr))
		goto cleanup;

	UINT32 submitted;
	hr = SubmitIoRing(state->global_context->io_ring, 0, 1, &submitted);
	if (FAILED(hr))
		goto cleanup;

	return (AsyncResult){ .state = state,
						  .poll = poll_ring_write,
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
									 .message = "Ring write setup failed" } };
}
