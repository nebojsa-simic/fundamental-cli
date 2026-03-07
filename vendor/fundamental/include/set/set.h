#ifndef LIBRARY_SET_H
#define LIBRARY_SET_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "../error/error.h"
#include "../memory/memory.h"
#include "../collections/utils.h"

// Set entry node
typedef struct SetEntry {
	void *value;
	struct SetEntry *next;
} SetEntry;

// Core HashSet structure - type-agnostic
typedef struct {
	SetEntry **buckets;
	size_t bucket_count;
	size_t entry_count;
	size_t value_size;
	HashFunction hash_fn;
	KeyEqualFunction equals_fn;
} HashSet;

// Result type for Set operations
typedef struct {
	HashSet value;
	ErrorResult error;
} HashSetResult;

// Error codes specific to Set (must fit in uint8_t: 0-255)
#define ERROR_CODE_ELEMENT_EXISTS 41
#define ERROR_CODE_ELEMENT_NOT_FOUND 42

// Core Set API - type-agnostic operations
HashSetResult fun_set_create(size_t value_size, size_t initial_bucket_count,
							 HashFunction hash_fn, KeyEqualFunction equals_fn);
ErrorResult fun_set_add(HashSet *set, const void *value);
ErrorResult fun_set_contains(const HashSet *set, const void *value,
							 bool *out_contains);
ErrorResult fun_set_remove(HashSet *set, const void *value);
size_t fun_set_size(const HashSet *set);
ErrorResult fun_set_destroy(HashSet *set);

// Macro to define type-safe Set with custom hash/equals
// Usage: DEFINE_SET_TYPE_CUSTOM(Point, fun_hash_Point, fun_equals_Point)
#define DEFINE_SET_TYPE_CUSTOM(T, HASH_FN, EQUALS_FN)                    \
	typedef struct {                                                     \
		HashSet set;                                                     \
	} T##Set;                                                            \
                                                                         \
	typedef struct {                                                     \
		T##Set value;                                                    \
		ErrorResult error;                                               \
	} T##SetResult;                                                      \
                                                                         \
	static inline T##SetResult fun_set_##T##_create(size_t bucket_count) \
	{                                                                    \
		T##SetResult result;                                             \
		HashSetResult set_result =                                       \
			fun_set_create(sizeof(T), bucket_count, HASH_FN, EQUALS_FN); \
		result.error = set_result.error;                                 \
		result.value.set = set_result.value;                             \
		return result;                                                   \
	}                                                                    \
                                                                         \
	static inline ErrorResult fun_set_##T##_add(T##Set *set, T value)    \
	{                                                                    \
		return fun_set_add(&set->set, &value);                           \
	}                                                                    \
                                                                         \
	static inline ErrorResult fun_set_##T##_contains(const T##Set *set,  \
													 T value, bool *out) \
	{                                                                    \
		return fun_set_contains(&set->set, &value, out);                 \
	}                                                                    \
                                                                         \
	static inline ErrorResult fun_set_##T##_remove(T##Set *set, T value) \
	{                                                                    \
		return fun_set_remove(&set->set, &value);                        \
	}                                                                    \
                                                                         \
	static inline size_t fun_set_##T##_size(const T##Set *set)           \
	{                                                                    \
		return fun_set_size(&set->set);                                  \
	}                                                                    \
                                                                         \
	static inline ErrorResult fun_set_##T##_destroy(T##Set *set)         \
	{                                                                    \
		return fun_set_destroy(&set->set);                               \
	}

// Convenience macro for primitive types with existing hash/equals
// Usage: DEFINE_SET_TYPE(int)
#define DEFINE_SET_TYPE(T) \
	DEFINE_SET_TYPE_CUSTOM(T, fun_hash_##T, fun_equals_##T)

#endif // LIBRARY_SET_H
