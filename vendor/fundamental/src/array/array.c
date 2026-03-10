#include "../../include/array/array.h"

// Implementation of the core type-agnostic Array functions

ArrayResult fun_array_create(size_t element_size, size_t initial_capacity) {
  ArrayResult result = {.error = ERROR_RESULT_NO_ERROR};

  if (initial_capacity == 0) {
    initial_capacity = 1; // Minimum capacity
  }

  MemoryResult mem_result =
      fun_memory_allocate(initial_capacity * element_size);
  if (fun_error_is_error(mem_result.error)) {
    result.error = mem_result.error;
    return result;
  }

  // Set up the array structure
  result.value.data = mem_result.value;
  result.value.count = 0;
  result.value.capacity = initial_capacity;
  result.value.element_size = element_size;

  return result;
}

ErrorResult fun_array_push(Array *array, const void *element) {
  if (!array || !element) {
    return ERROR_RESULT_NULL_POINTER;
  }

  // Check if we need to grow the array
  if (array->count >= array->capacity) {
    // Calculate new capacity (double the size for amortized efficiency)
    size_t new_capacity = (array->capacity > 0) ? array->capacity * 2 : 1;

    MemoryResult new_block_result =
        fun_memory_reallocate(array->data, new_capacity * array->element_size);
    if (fun_error_is_error(new_block_result.error)) {
      return fun_error_result(ERROR_CODE_REALLOCATION_FAILED,
                              "Could not grow array");
    }

    array->data = new_block_result.value;
    array->capacity = new_capacity;
  }

  // Copy the element to the end of the array
  size_t pos = array->count;
  void *destptr = (char *)(array->data) + pos * array->element_size;
  // Simple byte copy
  for (size_t i = 0; i < array->element_size; i++) {
    ((char *)destptr)[i] = ((const char *)element)[i];
  }

  // Increase the count
  array->count++;
  return ERROR_RESULT_NO_ERROR;
}

ErrorResult fun_array_get(const Array *array, size_t index, void *out_element) {
  if (!array || !out_element) {
    return ERROR_RESULT_NULL_POINTER;
  }

  if (index >= array->count) {
    return fun_error_result(ERROR_CODE_INVALID_INDEX, "Index out of bounds");
  }

  // Copy the element from the array to output
  const void *srceptr =
      (const char *)(array->data) + index * array->element_size;
  // Simple byte copy
  for (size_t i = 0; i < array->element_size; i++) {
    ((char *)out_element)[i] = ((const char *)srceptr)[i];
  }

  return ERROR_RESULT_NO_ERROR;
}

ErrorResult fun_array_set(Array *array, size_t index, const void *element) {
  if (!array || !element) {
    return ERROR_RESULT_NULL_POINTER;
  }

  if (index >= array->count) {
    return fun_error_result(ERROR_CODE_INVALID_INDEX, "Index out of bounds");
  }

  // Copy the element to the array data
  void *destptr = (char *)(array->data) + index * array->element_size;
  // Simple byte copy
  for (size_t i = 0; i < array->element_size; i++) {
    ((char *)destptr)[i] = ((const char *)element)[i];
  }

  return ERROR_RESULT_NO_ERROR;
}

size_t fun_array_size(const Array *array) {
  if (!array)
    return 0;
  return array->count;
}

size_t fun_array_capacity(const Array *array) {
  if (!array)
    return 0;
  return array->capacity;
}

ErrorResult fun_array_destroy(Array *array) {
  if (!array) {
    return ERROR_RESULT_NULL_POINTER;
  }

  if (array->data) {
    voidResult free_res = fun_memory_free((Memory *)&array->data);
    array->data = NULL;
    array->count = 0;
    array->capacity = 0;
    array->element_size = 0;
    return free_res.error;
  }

  return ERROR_RESULT_NO_ERROR;
}
