#include "fileRead.h"
#include "fileAdaptive.h"

#undef NTDDI_VERSION
#define NTDDI_VERSION NTDDI_WIN10_NI

#include "ioringcompat.h"
#include <ioringapi.h>

typedef struct {
	HIORING io_ring;
	IORING_CREATE_FLAGS flags;
	IORING_INFO info;
} IoRingContext;

typedef struct {
	Read parameters;
	IoRingContext *global_context;
	HANDLE file_handle;
	uint64_t request_id;
	AsyncStatus async_status;
} RingReadState;

static IoRingContext global_read_context = { 0 };
static uint64_t next_read_request_id = 0;

static inline ErrorResult initialize_io_ring(IoRingContext *ctx)
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

static inline AsyncStatus poll_ring_read(AsyncResult *result)
{
	RingReadState *state = result->state;
	IORING_CQE cqe;

	if (state->async_status != ASYNC_PENDING) {
		AsyncStatus status = state->async_status;
		FileAdaptiveState *adaptive = state->parameters.adaptive;
		uint64_t bytes = state->parameters.bytes_to_read;
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
			uint64_t bytes = state->parameters.bytes_to_read;
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

AsyncResult create_ring_read(Read parameters)
{
	MemoryResult mem_result = fun_memory_allocate(sizeof(RingReadState));
	if (fun_error_is_error(mem_result.error))
		return (AsyncResult){ .status = ASYNC_ERROR,
							  .error = mem_result.error };

	HANDLE file = INVALID_HANDLE_VALUE;
	RingReadState *state = (RingReadState *)mem_result.value;
	state->global_context = &global_read_context;
	state->request_id = next_read_request_id++;
	state->async_status = ASYNC_PENDING;
	state->parameters = parameters;

	if (!state->global_context->io_ring) {
		ErrorResult init = initialize_io_ring(state->global_context);
		if (fun_error_is_error(init))
			goto cleanup;
	}

	file = CreateFile(parameters.file_path, GENERIC_READ, FILE_SHARE_READ, NULL,
					  OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
	if (file == INVALID_HANDLE_VALUE)
		goto cleanup;

	state->file_handle = file;

	IORING_HANDLE_REF file_ref = IoRingHandleRefFromHandle(file);
	IORING_BUFFER_REF buffer_ref =
		IoRingBufferRefFromPointer(parameters.output);

	HRESULT hr = BuildIoRingReadFile(state->global_context->io_ring, file_ref,
									 buffer_ref, parameters.bytes_to_read,
									 parameters.offset, state->request_id,
									 IOSQE_FLAGS_NONE);
	if (FAILED(hr))
		goto cleanup;

	UINT32 submitted;
	hr = SubmitIoRing(state->global_context->io_ring, 0, 1, &submitted);
	if (FAILED(hr))
		goto cleanup;

	return (AsyncResult){ .state = state,
						  .poll = poll_ring_read,
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
									 .message = "Ring read setup failed" } };
}
