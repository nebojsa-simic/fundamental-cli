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

typedef struct {
	uint8_t code;
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
