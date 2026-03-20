#ifndef LIBRARY_HASHMAP_H
#define LIBRARY_HASHMAP_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "../error/error.h"
#include "../memory/memory.h"
#include "../collections/utils.h" // Reuse hash/equality utilities

// Hash function type for keys
typedef uint64_t (*HashFunction)(const void *key);
typedef bool (*KeyEqualFunction)(const void *key1, const void *key2);

// Helper macros for easy hash/equals function selection
// Usage: fun_hash_fn_for_type(int) expands to fun_hash_int
#define fun_hash_fn_for_type(T) fun_hash_##T
#define fun_equals_fn_for_type(T) fun_equals_##T

// HashMap entry node
typedef struct HashMapEntry {
	void *key;
	void *value;
	struct HashMapEntry *next; // For collision chaining
} HashMapEntry;

// Core HashMap structure - type-agnostic
typedef struct {
	HashMapEntry **buckets; // Array of bucket pointers
	size_t bucket_count; // Number of buckets
	size_t entry_count; // Total entries stored
	size_t key_size; // Size of key type
	size_t value_size; // Size of value type
	HashFunction hash_fn; // Hash function for keys
	KeyEqualFunction equals_fn; // Equality function for keys
} HashMap;

// Result type for HashMap operations
typedef struct {
	HashMap value;
	ErrorResult error;
} HashMapResult;

// Error codes specific to HashMap
#define ERROR_CODE_KEY_NOT_FOUND 201
#define ERROR_CODE_HASHMAP_FULL 202
#define ERROR_CODE_INVALID_HASH 203

// Core HashMap API - type-agnostic operations
HashMapResult fun_hashmap_create(size_t key_size, size_t value_size,
								 size_t initial_bucket_count,
								 HashFunction hash_fn,
								 KeyEqualFunction equals_fn);
ErrorResult fun_hashmap_put(HashMap *map, const void *key, const void *value);
ErrorResult fun_hashmap_get(const HashMap *map, const void *key,
							void *out_value);
ErrorResult fun_hashmap_remove(HashMap *map, const void *key);
ErrorResult fun_hashmap_contains(const HashMap *map, const void *key,
								 bool *out_contains);
size_t fun_hashmap_size(const HashMap *map);
ErrorResult fun_hashmap_destroy(HashMap *map);

// Macro to define type-safe HashMap with custom hash/equals functions
// Usage: DEFINE_HASHMAP_TYPE_CUSTOM(Point, int, fun_hash_Point, fun_equals_Point)
#define DEFINE_HASHMAP_TYPE_CUSTOM(K, V, HASH_FN, EQUALS_FN)                  \
	typedef struct {                                                          \
		HashMap map;                                                          \
	} K##V##HashMap;                                                          \
                                                                              \
	typedef struct {                                                          \
		K##V##HashMap value;                                                  \
		ErrorResult error;                                                    \
	} K##V##HashMapResult;                                                    \
                                                                              \
	static inline K##V##HashMapResult fun_hashmap_##K##_##V##_create(         \
		size_t initial_bucket_count)                                          \
	{                                                                         \
		K##V##HashMapResult result;                                           \
		HashMapResult map_result = fun_hashmap_create(                        \
			sizeof(K), sizeof(V), initial_bucket_count, HASH_FN, EQUALS_FN);  \
		result.error = map_result.error;                                      \
		result.value.map = map_result.value;                                  \
		return result;                                                        \
	}                                                                         \
                                                                              \
	static inline ErrorResult fun_hashmap_##K##_##V##_put(K##V##HashMap *map, \
														  K key, V value)     \
	{                                                                         \
		return fun_hashmap_put(&map->map, &key, &value);                      \
	}                                                                         \
                                                                              \
	static inline V fun_hashmap_##K##_##V##_get(const K##V##HashMap *map,     \
												K key)                        \
	{                                                                         \
		V value;                                                              \
		ErrorResult result = fun_hashmap_get(&map->map, &key, &value);        \
		(void)result;                                                         \
		return value;                                                         \
	}                                                                         \
                                                                              \
	static inline ErrorResult fun_hashmap_##K##_##V##_contains(               \
		const K##V##HashMap *map, K key, bool *out)                           \
	{                                                                         \
		return fun_hashmap_contains(&map->map, &key, out);                    \
	}                                                                         \
                                                                              \
	static inline ErrorResult fun_hashmap_##K##_##V##_remove(                 \
		K##V##HashMap *map, K key)                                            \
	{                                                                         \
		return fun_hashmap_remove(&map->map, &key);                           \
	}                                                                         \
                                                                              \
	static inline size_t fun_hashmap_##K##_##V##_size(                        \
		const K##V##HashMap *map)                                             \
	{                                                                         \
		return fun_hashmap_size(&map->map);                                   \
	}                                                                         \
                                                                              \
	static inline ErrorResult fun_hashmap_##K##_##V##_destroy(                \
		K##V##HashMap *map)                                                   \
	{                                                                         \
		return fun_hashmap_destroy(&map->map);                                \
	}

// For simple POD structs, use byte-wise hash and memcmp equality
// Usage: DEFINE_HASHMAP_TYPE_STRUCT(Point, int) for struct Point { int x; int y; }
#define DEFINE_HASHMAP_TYPE_STRUCT(K, V)                                     \
	DEFINE_HASHMAP_TYPE_CUSTOM(K, V, (const HashFunction)fun_hash_bytes_##K, \
							   (const KeyEqualFunction)fun_equals_bytes_##K) \
                                                                             \
	static inline uint64_t fun_hash_bytes_##K(const void *key)               \
	{                                                                        \
		return fun_hash_bytes(key, sizeof(K));                               \
	}                                                                        \
	static inline bool fun_equals_bytes_##K(const void *k1, const void *k2)  \
	{                                                                        \
		return fun_equals_bytes(k1, k2, sizeof(K));                          \
	}

// Macro to define type-safe HashMap operations for any Key/Value type pair
// Works for primitive types that have fun_hash_* and fun_equals_* defined
// Usage: DEFINE_HASHMAP_TYPE(int, char) creates IntCharHashMap, fun_hashmap_IntChar_*, etc.
#define DEFINE_HASHMAP_TYPE(K, V)                                             \
	typedef struct {                                                          \
		HashMap map;                                                          \
	} K##V##HashMap;                                                          \
                                                                              \
	typedef struct {                                                          \
		K##V##HashMap value;                                                  \
		ErrorResult error;                                                    \
	} K##V##HashMapResult;                                                    \
                                                                              \
	static inline K##V##HashMapResult fun_hashmap_##K##_##V##_create(         \
		size_t initial_bucket_count)                                          \
	{                                                                         \
		K##V##HashMapResult result;                                           \
		HashMapResult map_result =                                            \
			fun_hashmap_create(sizeof(K), sizeof(V), initial_bucket_count,    \
							   fun_hash_##K, fun_equals_##K);                 \
		result.error = map_result.error;                                      \
		result.value.map = map_result.value;                                  \
		return result;                                                        \
	}                                                                         \
                                                                              \
	static inline ErrorResult fun_hashmap_##K##_##V##_put(K##V##HashMap *map, \
														  K key, V value)     \
	{                                                                         \
		return fun_hashmap_put(&map->map, &key, &value);                      \
	}                                                                         \
                                                                              \
	static inline V fun_hashmap_##K##_##V##_get(const K##V##HashMap *map,     \
												K key)                        \
	{                                                                         \
		V value;                                                              \
		ErrorResult result = fun_hashmap_get(&map->map, &key, &value);        \
		(void)result;                                                         \
		return value;                                                         \
	}                                                                         \
                                                                              \
	static inline ErrorResult fun_hashmap_##K##_##V##_contains(               \
		const K##V##HashMap *map, K key, bool *out)                           \
	{                                                                         \
		return fun_hashmap_contains(&map->map, &key, out);                    \
	}                                                                         \
                                                                              \
	static inline ErrorResult fun_hashmap_##K##_##V##_remove(                 \
		K##V##HashMap *map, K key)                                            \
	{                                                                         \
		return fun_hashmap_remove(&map->map, &key);                           \
	}                                                                         \
                                                                              \
	static inline size_t fun_hashmap_##K##_##V##_size(                        \
		const K##V##HashMap *map)                                             \
	{                                                                         \
		return fun_hashmap_size(&map->map);                                   \
	}                                                                         \
                                                                              \
	static inline ErrorResult fun_hashmap_##K##_##V##_destroy(                \
		K##V##HashMap *map)                                                   \
	{                                                                         \
		return fun_hashmap_destroy(&map->map);                                \
	}

// Convenience macro for String keys
#define DEFINE_HASHMAP_STRING_KEY(V)                                        \
	typedef struct {                                                        \
		HashMap map;                                                        \
	} String##V##HashMap;                                                   \
                                                                            \
	typedef struct {                                                        \
		String##V##HashMap value;                                           \
		ErrorResult error;                                                  \
	} String##V##HashMapResult;                                             \
                                                                            \
	static inline String##V##HashMapResult fun_hashmap_String_##V##_create( \
		size_t bucket_count)                                                \
	{                                                                       \
		String##V##HashMapResult result;                                    \
		HashMapResult map_result =                                          \
			fun_hashmap_create(sizeof(char *), sizeof(V), bucket_count,     \
							   fun_hash_string, fun_equals_string);         \
		result.error = map_result.error;                                    \
		result.value.map = map_result.value;                                \
		return result;                                                      \
	}                                                                       \
                                                                            \
	static inline ErrorResult fun_hashmap_String_##V##_put(                 \
		String##V##HashMap *map, char *key, V value)                        \
	{                                                                       \
		return fun_hashmap_put(&map->map, &key, &value);                    \
	}                                                                       \
                                                                            \
	static inline V fun_hashmap_String_##V##_get(                           \
		const String##V##HashMap *map, char *key)                           \
	{                                                                       \
		V value;                                                            \
		ErrorResult result = fun_hashmap_get(&map->map, &key, &value);      \
		(void)result;                                                       \
		return value;                                                       \
	}                                                                       \
                                                                            \
	static inline ErrorResult fun_hashmap_String_##V##_contains(            \
		const String##V##HashMap *map, char *key, bool *out)                \
	{                                                                       \
		return fun_hashmap_contains(&map->map, &key, out);                  \
	}                                                                       \
                                                                            \
	static inline ErrorResult fun_hashmap_String_##V##_remove(              \
		String##V##HashMap *map, char *key)                                 \
	{                                                                       \
		return fun_hashmap_remove(&map->map, &key);                         \
	}                                                                       \
                                                                            \
	static inline size_t fun_hashmap_String_##V##_size(                     \
		const String##V##HashMap *map)                                      \
	{                                                                       \
		return fun_hashmap_size(&map->map);                                 \
	}                                                                       \
                                                                            \
	static inline ErrorResult fun_hashmap_String_##V##_destroy(             \
		String##V##HashMap *map)                                            \
	{                                                                       \
		return fun_hashmap_destroy(&map->map);                              \
	}

#endif // LIBRARY_HASHMAP_H
