#pragma once
#include "../error/error.h"
#include "../async/async.h"
#include "../memory/memory.h"
#include "../file/file.h"

typedef enum {
	STREAM_MODE_READ = 0,
	STREAM_MODE_WRITE = 1,
	STREAM_MODE_APPEND = 2
} StreamMode;

#define STREAM_READ_ONLY STREAM_MODE_READ
#define STREAM_WRITE_ONLY STREAM_MODE_WRITE
#define STREAM_APPEND_ONLY STREAM_MODE_APPEND

typedef struct {
	String file_path; // REQUIRED - Target file path
	StreamMode mode; // REQUIRED - Stream direction
	Memory buffer; // REQUIRED - Caller-allocated buffer
	uint64_t buffer_size; // REQUIRED - Buffer capacity
	uint64_t current_position; // Status - Current file position
	uint64_t bytes_processed; // Status - Total bytes handled
	bool end_of_stream; // Status - Stream completion
	bool has_data_available; // Status - Data ready for processing
	void *internal_state; // Internal - Platform-specific state
} FileStream;

DEFINE_RESULT_TYPE(FileStream);

// Lifecycle operations
AsyncResult fun_stream_open(String file_path, StreamMode mode, Memory buffer,
							uint64_t buffer_size, FileMode file_mode);
AsyncResult fun_stream_create_file_read(String file_path, Memory buffer,
										uint64_t buffer_size, FileMode mode);
AsyncResult fun_stream_destroy(FileStream *stream);
AsyncResult fun_stream_close(FileStream *stream);

// I/O operations
AsyncResult fun_stream_read(FileStream *stream, uint64_t *bytes_read);
AsyncResult fun_stream_write(FileStream *stream, Memory data,
							 uint64_t data_size);

// Status and control
bool fun_stream_can_read(FileStream *stream);
bool fun_stream_can_write(FileStream *stream, uint64_t requested_size);
bool fun_stream_is_end_of_stream(FileStream *stream);
uint64_t fun_stream_current_position(FileStream *stream);
