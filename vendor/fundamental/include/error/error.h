#ifndef LIBRARY_ERROR_RESULT_H
#define LIBRARY_ERROR_RESULT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

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

typedef struct {
  uint8_t code;
  const char *message;
} ErrorResult;

#define DEFINE_RESULT_TYPE(T)                                                  \
  typedef struct {                                                             \
    T value;                                                                   \
    ErrorResult error;                                                         \
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
static inline ErrorResult fun_error_result(uint8_t code, const char *message) {
  ErrorResult result = {code, message};
  return result;
}

// Standard errors
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"

static ErrorResult ERROR_RESULT_NO_ERROR = {ERROR_CODE_NO_ERROR, NULL};
static ErrorResult ERROR_RESULT_NULL_POINTER = {ERROR_CODE_NULL_POINTER,
                                                "Null pointer provided"};
static ErrorResult ERROR_RESULT_DIRECTORY_EXISTS = {ERROR_CODE_DIRECTORY_EXISTS,
                                                    "Directory already exists"};
static ErrorResult ERROR_RESULT_DIRECTORY_NOT_FOUND = {
    ERROR_CODE_DIRECTORY_NOT_FOUND, "Directory not found"};
static ErrorResult ERROR_RESULT_DIRECTORY_NOT_EMPTY = {
    ERROR_CODE_DIRECTORY_NOT_EMPTY, "Directory is not empty"};
static ErrorResult ERROR_RESULT_NOT_DIRECTORY = {ERROR_CODE_NOT_DIRECTORY,
                                                 "Path is not a directory"};
static ErrorResult ERROR_RESULT_PATH_INVALID = {ERROR_CODE_PATH_INVALID,
                                                "Invalid path"};
static ErrorResult ERROR_RESULT_PERMISSION_DENIED = {
    ERROR_CODE_PERMISSION_DENIED, "Permission denied"};
static ErrorResult ERROR_RESULT_BUFFER_TOO_SMALL = {ERROR_CODE_BUFFER_TOO_SMALL,
                                                    "Buffer too small"};

#pragma GCC diagnostic pop

static inline bool fun_error_is_error(ErrorResult error) {
  return error.code != ERROR_CODE_NO_ERROR;
}

static inline bool fun_error_is_ok(ErrorResult error) {
  return error.code == ERROR_CODE_NO_ERROR;
}

#endif // LIBRARY_ERROR_RESULT_H
