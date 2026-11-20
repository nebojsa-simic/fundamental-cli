#ifndef LIBRARY_ERROR_RESULT_H
#define LIBRARY_ERROR_RESULT_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define ERROR_CODE_NO_ERROR 0
#define ERROR_CODE_NULL_POINTER 1
#define ERROR_CODE_PROCESS_SPAWN_FAILED 2
#define ERROR_CODE_PROCESS_NOT_FOUND 3
#define ERROR_CODE_PROCESS_TERMINATE_FAILED 4
#define ERROR_CODE_DIRECTORY_EXISTS 5
#define ERROR_CODE_DIRECTORY_NOT_FOUND 6
#define ERROR_CODE_DIRECTORY_NOT_EMPTY 7
#define ERROR_CODE_NOT_DIRECTORY 8
#define ERROR_CODE_PATH_INVALID 9
#define ERROR_CODE_PERMISSION_DENIED 10
#define ERROR_CODE_BUFFER_TOO_SMALL 11
#define ERROR_CODE_INDEX_OUT_OF_BOUNDS 12
#define ERROR_CODE_PATH_TOO_LONG 13
#define ERROR_CODE_INTEGER_OVERFLOW 14
#define ERROR_CODE_LOCK_TIMEOUT 15
#define ERROR_CODE_CONFIG_KEY_NOT_FOUND 220
#define ERROR_CODE_CONFIG_PARSE_ERROR 221
#define ERROR_CODE_CONFIG_INVALID_APP_NAME 222

#define ERROR_CODE_NETWORK_INIT_FAILED 230
#define ERROR_CODE_NETWORK_CONNECT_FAILED 231
#define ERROR_CODE_NETWORK_CONNECT_TIMEOUT 232
#define ERROR_CODE_NETWORK_SEND_FAILED 233
#define ERROR_CODE_NETWORK_WOULD_BLOCK 234
#define ERROR_CODE_NETWORK_RECEIVE_FAILED 235
#define ERROR_CODE_NETWORK_BIND_FAILED 236
#define ERROR_CODE_NETWORK_CLOSED 237
#define ERROR_CODE_NETWORK_ADDRESS_PARSE_FAILED 238
#define ERROR_CODE_NETWORK_INVALID_STATE 239
#define ERROR_CODE_NETWORK_SERVER_BIND_FAILED 240
#define ERROR_CODE_NETWORK_SERVER_WRONG_CONFIG_TYPE 241

#define ERROR_CODE_ASYNC_TIMEOUT 242

#define ERROR_CODE_THREAD_POOL_INVALID_SIZE 250
#define ERROR_CODE_THREAD_POOL_CREATE_FAILED 251
#define ERROR_CODE_THREAD_POOL_FULL 252

#define ERROR_CODE_JSON_PARSE_ERROR 270
#define ERROR_CODE_JSON_UNTERMINATED_STRING 271
#define ERROR_CODE_JSON_INVALID_NUMBER 272
#define ERROR_CODE_JSON_NESTING_TOO_DEEP 273
#define ERROR_CODE_JSON_MISSING_COLON 275
#define ERROR_CODE_JSON_UNEXPECTED_TOKEN 276
#define ERROR_CODE_JSON_MISSING_COMMA 277
#define ERROR_CODE_JSON_PATH_NOT_FOUND 278
#define ERROR_CODE_JSON_TYPE_MISMATCH 279
#define ERROR_CODE_JSON_INCOMPLETE 280

typedef struct {
	uint16_t code;
	const char *message;
} ErrorResult;

#define DEFINE_RESULT_TYPE(T) \
	typedef struct {          \
		T value;              \
		ErrorResult error;    \
	} T##Result

// Define result types for common simple types
DEFINE_RESULT_TYPE(bool);
DEFINE_RESULT_TYPE(char);
DEFINE_RESULT_TYPE(float);
DEFINE_RESULT_TYPE(double);
DEFINE_RESULT_TYPE(int8_t);
DEFINE_RESULT_TYPE(uint8_t);
DEFINE_RESULT_TYPE(int16_t);
DEFINE_RESULT_TYPE(uint16_t);
DEFINE_RESULT_TYPE(int32_t);
DEFINE_RESULT_TYPE(uint32_t);
DEFINE_RESULT_TYPE(int64_t);
DEFINE_RESULT_TYPE(uint64_t);
DEFINE_RESULT_TYPE(size_t);

// Special case for void
typedef struct {
	ErrorResult error;
} voidResult;

// Indicates that the function returns an error
#define CanReturnError(...) __VA_ARGS__##Result

// Helper functions for error creation and checking
static inline ErrorResult fun_error_result(uint8_t code, const char *message)
{
	ErrorResult result = { code, message };
	return result;
}

// Standard errors
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"

static ErrorResult ERROR_RESULT_NO_ERROR = { ERROR_CODE_NO_ERROR, NULL };
static ErrorResult ERROR_RESULT_NULL_POINTER = { ERROR_CODE_NULL_POINTER,
												 "Null pointer provided" };
static ErrorResult ERROR_RESULT_DIRECTORY_EXISTS = {
	ERROR_CODE_DIRECTORY_EXISTS, "Directory already exists"
};
static ErrorResult ERROR_RESULT_DIRECTORY_NOT_FOUND = {
	ERROR_CODE_DIRECTORY_NOT_FOUND, "Directory not found"
};
static ErrorResult ERROR_RESULT_DIRECTORY_NOT_EMPTY = {
	ERROR_CODE_DIRECTORY_NOT_EMPTY, "Directory is not empty"
};
static ErrorResult ERROR_RESULT_NOT_DIRECTORY = { ERROR_CODE_NOT_DIRECTORY,
												  "Path is not a directory" };
static ErrorResult ERROR_RESULT_PATH_INVALID = { ERROR_CODE_PATH_INVALID,
												 "Invalid path" };
static ErrorResult ERROR_RESULT_PERMISSION_DENIED = {
	ERROR_CODE_PERMISSION_DENIED, "Permission denied"
};
static ErrorResult ERROR_RESULT_BUFFER_TOO_SMALL = {
	ERROR_CODE_BUFFER_TOO_SMALL, "Buffer too small"
};
static ErrorResult ERROR_RESULT_PATH_TOO_LONG = { ERROR_CODE_PATH_TOO_LONG,
												  "Path exceeds buffer" };
static ErrorResult ERROR_RESULT_INTEGER_OVERFLOW = {
	ERROR_CODE_INTEGER_OVERFLOW, "Integer overflow in size calculation"
};
static ErrorResult ERROR_RESULT_LOCK_TIMEOUT = { ERROR_CODE_LOCK_TIMEOUT,
												 "File lock timed out" };
static ErrorResult ERROR_RESULT_CONFIG_KEY_NOT_FOUND = {
	ERROR_CODE_CONFIG_KEY_NOT_FOUND, "Configuration key not found"
};
static ErrorResult ERROR_RESULT_CONFIG_PARSE_ERROR = {
	ERROR_CODE_CONFIG_PARSE_ERROR, "Failed to parse configuration value"
};
static ErrorResult ERROR_RESULT_CONFIG_INVALID_APP_NAME = {
	ERROR_CODE_CONFIG_INVALID_APP_NAME, "Invalid app name (null or empty)"
};
static ErrorResult ERROR_RESULT_NETWORK_INIT_FAILED = {
	ERROR_CODE_NETWORK_INIT_FAILED, "Network loop initialisation failed"
};
static ErrorResult ERROR_RESULT_NETWORK_CONNECT_FAILED = {
	ERROR_CODE_NETWORK_CONNECT_FAILED, "TCP connection failed"
};
static ErrorResult ERROR_RESULT_NETWORK_CONNECT_TIMEOUT = {
	ERROR_CODE_NETWORK_CONNECT_TIMEOUT, "TCP connection timed out"
};
static ErrorResult ERROR_RESULT_NETWORK_SEND_FAILED = {
	ERROR_CODE_NETWORK_SEND_FAILED, "Network send failed"
};
static ErrorResult ERROR_RESULT_NETWORK_WOULD_BLOCK = {
	ERROR_CODE_NETWORK_WOULD_BLOCK,
	"Send buffer full; wait for on_write_complete"
};
static ErrorResult ERROR_RESULT_NETWORK_RECEIVE_FAILED = {
	ERROR_CODE_NETWORK_RECEIVE_FAILED, "Network receive failed"
};
static ErrorResult ERROR_RESULT_NETWORK_BIND_FAILED = {
	ERROR_CODE_NETWORK_BIND_FAILED, "UDP socket bind failed"
};
static ErrorResult ERROR_RESULT_NETWORK_CLOSED = { ERROR_CODE_NETWORK_CLOSED,
												   "Connection is closed" };
static ErrorResult ERROR_RESULT_NETWORK_ADDRESS_PARSE_FAILED = {
	ERROR_CODE_NETWORK_ADDRESS_PARSE_FAILED, "Failed to parse network address"
};
static ErrorResult ERROR_RESULT_NETWORK_INVALID_STATE = {
	ERROR_CODE_NETWORK_INVALID_STATE, "Connection is in an invalid state"
};
static ErrorResult ERROR_RESULT_NETWORK_SERVER_BIND_FAILED = {
	ERROR_CODE_NETWORK_SERVER_BIND_FAILED, "Server socket bind failed"
};
static ErrorResult ERROR_RESULT_NETWORK_SERVER_WRONG_CONFIG_TYPE = {
	ERROR_CODE_NETWORK_SERVER_WRONG_CONFIG_TYPE,
	"Config type does not match listen function"
};
static ErrorResult ERROR_RESULT_ASYNC_TIMEOUT = { ERROR_CODE_ASYNC_TIMEOUT,
												  "Async operation timed out" };
static ErrorResult ERROR_RESULT_THREAD_POOL_INVALID_SIZE = {
	ERROR_CODE_THREAD_POOL_INVALID_SIZE, "Thread pool requires num_threads > 0"
};
static ErrorResult ERROR_RESULT_THREAD_POOL_CREATE_FAILED = {
	ERROR_CODE_THREAD_POOL_CREATE_FAILED, "Failed to create worker thread"
};
static ErrorResult ERROR_RESULT_THREAD_POOL_FULL = {
	ERROR_CODE_THREAD_POOL_FULL, "All worker threads are busy"
};
static ErrorResult ERROR_RESULT_JSON_PARSE_ERROR = {
	ERROR_CODE_JSON_PARSE_ERROR, "JSON parse error"
};
static ErrorResult ERROR_RESULT_JSON_UNTERMINATED_STRING = {
	ERROR_CODE_JSON_UNTERMINATED_STRING, "Unterminated string literal"
};
static ErrorResult ERROR_RESULT_JSON_INVALID_NUMBER = {
	ERROR_CODE_JSON_INVALID_NUMBER, "Invalid number format"
};
static ErrorResult ERROR_RESULT_JSON_NESTING_TOO_DEEP = {
	ERROR_CODE_JSON_NESTING_TOO_DEEP, "JSON nesting too deep"
};
static ErrorResult ERROR_RESULT_JSON_MISSING_COLON = {
	ERROR_CODE_JSON_MISSING_COLON, "Missing colon after key"
};
static ErrorResult ERROR_RESULT_JSON_UNEXPECTED_TOKEN = {
	ERROR_CODE_JSON_UNEXPECTED_TOKEN, "Unexpected token"
};
static ErrorResult ERROR_RESULT_JSON_MISSING_COMMA = {
	ERROR_CODE_JSON_MISSING_COMMA, "Missing comma between elements"
};
static ErrorResult ERROR_RESULT_JSON_PATH_NOT_FOUND = {
	ERROR_CODE_JSON_PATH_NOT_FOUND, "Path not found in JSON document"
};
static ErrorResult ERROR_RESULT_JSON_TYPE_MISMATCH = {
	ERROR_CODE_JSON_TYPE_MISMATCH, "Type mismatch in JSON value"
};
static ErrorResult ERROR_RESULT_JSON_INCOMPLETE = { ERROR_CODE_JSON_INCOMPLETE,
													"Need more data" };

#pragma GCC diagnostic pop

static inline bool fun_error_is_error(ErrorResult error)
{
	return error.code != ERROR_CODE_NO_ERROR;
}

static inline bool fun_error_is_ok(ErrorResult error)
{
	return error.code == ERROR_CODE_NO_ERROR;
}

#endif // LIBRARY_ERROR_RESULT_H
