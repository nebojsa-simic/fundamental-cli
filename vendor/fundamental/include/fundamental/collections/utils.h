#ifndef LIBRARY_COLLECTIONS_UTILS_H
#define LIBRARY_COLLECTIONS_UTILS_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "../error/error.h"
#include "../memory/memory.h"

// Hash/Equality function types for collections
typedef uint64_t (*HashFunction)(const void *key);
typedef bool (*KeyEqualFunction)(const void *key1, const void *key2);

// ============================================================================
// Generic Hash Functions
// ============================================================================

// Hash for primitive integer types
static inline uint64_t fun_collections_hash_int(const void *key)
{
	return (uint64_t)(*(const int *)key);
}

static inline uint64_t fun_collections_hash_int32(const void *key)
{
	return (uint64_t)(*(const int32_t *)key);
}

static inline uint64_t fun_collections_hash_int64(const void *key)
{
	return (uint64_t)(*(const int64_t *)key);
}

static inline uint64_t fun_collections_hash_uint32(const void *key)
{
	return (uint64_t)(*(const uint32_t *)key);
}

static inline uint64_t fun_collections_hash_uint64(const void *key)
{
	return *(const uint64_t *)key;
}

// Hash for pointer types (uses pointer value as hash)
static inline uint64_t fun_collections_hash_ptr(const void *key)
{
	return (uint64_t)(uintptr_t)key;
}

// Hash for char* strings (FNV-1a algorithm)
static inline uint64_t fun_collections_hash_string(const void *key)
{
	const char *str = (const char *)key;
	uint64_t hash = 14695981039346656037ULL;
	while (*str) {
		hash ^= (uint8_t)*str++;
		hash *= 1099511628211ULL;
	}
	return hash;
}

// Generic byte-wise hash for arbitrary data
static inline uint64_t fun_collections_hash_bytes(const void *key, size_t size)
{
	const uint8_t *bytes = (const uint8_t *)key;
	uint64_t hash = 0;
	for (size_t i = 0; i < size; i++) {
		hash = hash * 31 + bytes[i];
	}
	return hash;
}

// ============================================================================
// Generic Equality Functions
// ============================================================================

static inline bool fun_collections_equals_int(const void *k1, const void *k2)
{
	return *(const int *)k1 == *(const int *)k2;
}

static inline bool fun_collections_equals_int32(const void *k1, const void *k2)
{
	return *(const int32_t *)k1 == *(const int32_t *)k2;
}

static inline bool fun_collections_equals_int64(const void *k1, const void *k2)
{
	return *(const int64_t *)k1 == *(const int64_t *)k2;
}

static inline bool fun_collections_equals_uint32(const void *k1, const void *k2)
{
	return *(const uint32_t *)k1 == *(const uint32_t *)k2;
}

static inline bool fun_collections_equals_uint64(const void *k1, const void *k2)
{
	return *(const uint64_t *)k1 == *(const uint64_t *)k2;
}

static inline bool fun_collections_equals_ptr(const void *k1, const void *k2)
{
	return k1 == k2;
}

static inline bool fun_collections_equals_string(const void *k1, const void *k2)
{
	const char *s1 = (const char *)k1;
	const char *s2 = (const char *)k2;
	while (*s1 && (*s1 == *s2)) {
		s1++;
		s2++;
	}
	return *s1 == *s2;
}

static inline bool fun_collections_equals_bytes(const void *k1, const void *k2,
												size_t size)
{
	const uint8_t *p1 = (const uint8_t *)k1;
	const uint8_t *p2 = (const uint8_t *)k2;
	for (size_t i = 0; i < size; i++) {
		if (p1[i] != p2[i]) {
			return false;
		}
	}
	return true;
}

// ============================================================================
// Backwards Compatibility Aliases (shorter names for convenience)
// ============================================================================

#define fun_hash_int fun_collections_hash_int
#define fun_hash_int32 fun_collections_hash_int32
#define fun_hash_int64 fun_collections_hash_int64
#define fun_hash_uint32 fun_collections_hash_uint32
#define fun_hash_uint64 fun_collections_hash_uint64
#define fun_hash_ptr fun_collections_hash_ptr
#define fun_hash_string fun_collections_hash_string
#define fun_hash_bytes fun_collections_hash_bytes

#define fun_equals_int fun_collections_equals_int
#define fun_equals_int32 fun_collections_equals_int32
#define fun_equals_int64 fun_collections_equals_int64
#define fun_equals_uint32 fun_collections_equals_uint32
#define fun_equals_uint64 fun_collections_equals_uint64
#define fun_equals_ptr fun_collections_equals_ptr
#define fun_equals_string fun_collections_equals_string
#define fun_equals_bytes fun_collections_equals_bytes

#endif // LIBRARY_COLLECTIONS_UTILS_H
