#ifndef LIBRARY_ARRAY_H
#define LIBRARY_ARRAY_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "../error/error.h"
#include "../memory/memory.h"

// Core array structure - type-agnostic
typedef struct {
	void *data; // Dynamically allocated memory block
	size_t count; // Number of elements currently in array
	size_t capacity; // Maximum elements before resize
	size_t element_size; // Size of each element in bytes
} Array;

// Result type for generic array operations
typedef struct {
	Array value;
	ErrorResult error;
} ArrayResult;

// Define error codes specific to arrays
#define ERROR_CODE_INVALID_INDEX 101
#define ERROR_CODE_ARRAY_FULL 102
#define ERROR_CODE_REALLOCATION_FAILED 103

// Core Array API - type-agnostic operations
ArrayResult fun_array_create(size_t element_size, size_t initial_capacity);
ErrorResult fun_array_push(Array *array, const void *element);
ErrorResult fun_array_get(const Array *array, size_t index, void *out_element);
ErrorResult fun_array_set(Array *array, size_t index, const void *element);
size_t fun_array_size(const Array *array);
size_t fun_array_capacity(const Array *array);
ErrorResult fun_array_destroy(Array *array);

// Macro to define type-safe array operations for any type T
// Usage: DEFINE_ARRAY_TYPE(int) creates IntArray, fun_array_int_*, etc.
#define DEFINE_ARRAY_TYPE(T)                                                 \
	typedef struct {                                                         \
		Array array;                                                         \
	} T##Array;                                                              \
                                                                             \
	typedef struct {                                                         \
		T##Array value;                                                      \
		ErrorResult error;                                                   \
	} T##ArrayResult;                                                        \
                                                                             \
	static inline T##ArrayResult fun_array_##T##_create(                     \
		size_t initial_capacity)                                             \
	{                                                                        \
		T##ArrayResult result;                                               \
		ArrayResult array_result =                                           \
			fun_array_create(sizeof(T), initial_capacity);                   \
		result.error = array_result.error;                                   \
		result.value.array = array_result.value;                             \
		return result;                                                       \
	}                                                                        \
                                                                             \
	static inline ErrorResult fun_array_##T##_push(T##Array *array, T value) \
	{                                                                        \
		return fun_array_push(&array->array, &value);                        \
	}                                                                        \
                                                                             \
	static inline T fun_array_##T##_get(const T##Array *array, size_t index) \
	{                                                                        \
		T value;                                                             \
		ErrorResult result = fun_array_get(&array->array, index, &value);    \
		(void)result;                                                        \
		return value;                                                        \
	}                                                                        \
                                                                             \
	static inline ErrorResult fun_array_##T##_set(T##Array *array,           \
												  size_t index, T value)     \
	{                                                                        \
		return fun_array_set(&array->array, index, &value);                  \
	}                                                                        \
                                                                             \
	static inline size_t fun_array_##T##_size(const T##Array *array)         \
	{                                                                        \
		return fun_array_size(&array->array);                                \
	}                                                                        \
                                                                             \
	static inline ErrorResult fun_array_##T##_destroy(T##Array *array)       \
	{                                                                        \
		return fun_array_destroy(&array->array);                             \
	}

// Convenience macros for common usage
#define ARRAY_FOREACH(T, array_ptr, item_var)                                 \
	for (size_t _i = 0; _i < fun_array_##T##_size(array_ptr) &&               \
						((item_var) = fun_array_##T##_get(array_ptr, _i), 1); \
		 _i++)

#define ARRAY_FOREACH_IDX(T, array_ptr, item_var, idx_var)          \
	for (size_t idx_var = 0;                                        \
		 idx_var < fun_array_##T##_size(array_ptr) &&               \
		 ((item_var) = fun_array_##T##_get(array_ptr, idx_var), 1); \
		 idx_var++)

#endif // LIBRARY_ARRAY_H
