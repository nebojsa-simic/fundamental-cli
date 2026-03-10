#include "../../include/hashmap/hashmap.h"
#include "../../include/memory/memory.h"

// Internal helper: Calculate bucket index from hash
static inline size_t hashmap_bucket_index(const HashMap *map, uint64_t hash)
{
	return hash % map->bucket_count;
}

// Internal helper: Find entry by key in a bucket chain
static HashMapEntry *hashmap_find_entry_in_chain(HashMapEntry *chain,
												 const void *key,
												 KeyEqualFunction equals_fn)
{
	HashMapEntry *current = chain;
	while (current) {
		if (equals_fn(current->key, key)) {
			return current;
		}
		current = current->next;
	}
	return NULL;
}

HashMapResult fun_hashmap_create(size_t key_size, size_t value_size,
								 size_t initial_bucket_count,
								 HashFunction hash_fn,
								 KeyEqualFunction equals_fn)
{
	HashMapResult result = { .error = ERROR_RESULT_NO_ERROR };

	if (initial_bucket_count == 0) {
		initial_bucket_count = 16; // Default reasonable size
	}

	if (!hash_fn || !equals_fn) {
		result.error = fun_error_result(ERROR_CODE_INVALID_HASH,
										"Hash or equals function is NULL");
		return result;
	}

	// Allocate bucket array
	MemoryResult buckets_result =
		fun_memory_allocate(initial_bucket_count * sizeof(HashMapEntry *));
	if (fun_error_is_error(buckets_result.error)) {
		result.error = buckets_result.error;
		return result;
	}

	// Initialize buckets to NULL
	HashMapEntry **buckets = (HashMapEntry **)buckets_result.value;
	for (size_t i = 0; i < initial_bucket_count; i++) {
		buckets[i] = NULL;
	}

	// Set up the HashMap structure
	result.value.buckets = buckets;
	result.value.bucket_count = initial_bucket_count;
	result.value.entry_count = 0;
	result.value.key_size = key_size;
	result.value.value_size = value_size;
	result.value.hash_fn = hash_fn;
	result.value.equals_fn = equals_fn;

	return result;
}

ErrorResult fun_hashmap_put(HashMap *map, const void *key, const void *value)
{
	if (!map || !key || !value) {
		return ERROR_RESULT_NULL_POINTER;
	}

	// Calculate hash and bucket index
	uint64_t hash = map->hash_fn(key);
	size_t bucket_idx = hashmap_bucket_index(map, hash);

	// Check if key already exists (update case)
	HashMapEntry *existing = hashmap_find_entry_in_chain(
		map->buckets[bucket_idx], key, map->equals_fn);

	if (existing) {
		// Update existing value
		fun_memory_copy(value, existing->value, map->value_size);
		return ERROR_RESULT_NO_ERROR;
	}

	// Create new entry (insert case)
	MemoryResult entry_result = fun_memory_allocate(sizeof(HashMapEntry));
	if (fun_error_is_error(entry_result.error)) {
		return fun_error_result(ERROR_CODE_HASHMAP_FULL,
								"Failed to allocate new entry");
	}

	HashMapEntry *new_entry = (HashMapEntry *)entry_result.value;

	// Allocate and copy key
	MemoryResult key_result = fun_memory_allocate(map->key_size);
	if (fun_error_is_error(key_result.error)) {
		fun_memory_free((Memory *)&new_entry);
		return fun_error_result(ERROR_CODE_HASHMAP_FULL,
								"Failed to allocate key");
	}
	fun_memory_copy(key, key_result.value, map->key_size);

	// Allocate and copy value
	MemoryResult value_result = fun_memory_allocate(map->value_size);
	if (fun_error_is_error(value_result.error)) {
		fun_memory_free((Memory *)&key_result.value);
		fun_memory_free((Memory *)&new_entry);
		return fun_error_result(ERROR_CODE_HASHMAP_FULL,
								"Failed to allocate value");
	}
	fun_memory_copy(value, value_result.value, map->value_size);

	// Set up the new entry
	new_entry->key = key_result.value;
	new_entry->value = value_result.value;
	new_entry->next = map->buckets[bucket_idx]; // Prepend to chain

	// Insert at head of bucket chain
	map->buckets[bucket_idx] = new_entry;
	map->entry_count++;

	return ERROR_RESULT_NO_ERROR;
}

ErrorResult fun_hashmap_get(const HashMap *map, const void *key,
							void *out_value)
{
	if (!map || !key || !out_value) {
		return ERROR_RESULT_NULL_POINTER;
	}

	// Calculate hash and bucket index
	uint64_t hash = map->hash_fn(key);
	size_t bucket_idx = hashmap_bucket_index(map, hash);

	// Find entry in chain
	HashMapEntry *entry = hashmap_find_entry_in_chain(map->buckets[bucket_idx],
													  key, map->equals_fn);

	if (!entry) {
		return fun_error_result(ERROR_CODE_KEY_NOT_FOUND,
								"Key not found in hashmap");
	}

	// Copy value to output
	fun_memory_copy(entry->value, out_value, map->value_size);
	return ERROR_RESULT_NO_ERROR;
}

ErrorResult fun_hashmap_remove(HashMap *map, const void *key)
{
	if (!map || !key) {
		return ERROR_RESULT_NULL_POINTER;
	}

	// Calculate hash and bucket index
	uint64_t hash = map->hash_fn(key);
	size_t bucket_idx = hashmap_bucket_index(map, hash);

	// Find and remove from chain
	HashMapEntry *current = map->buckets[bucket_idx];
	HashMapEntry *prev = NULL;

	while (current) {
		if (map->equals_fn(current->key, key)) {
			// Found the entry to remove
			if (prev) {
				prev->next = current->next;
			} else {
				map->buckets[bucket_idx] = current->next;
			}

			// Free the entry's key and value
			fun_memory_free((Memory *)&current->key);
			fun_memory_free((Memory *)&current->value);
			fun_memory_free((Memory *)&current);

			map->entry_count--;
			return ERROR_RESULT_NO_ERROR;
		}
		prev = current;
		current = current->next;
	}

	return fun_error_result(ERROR_CODE_KEY_NOT_FOUND,
							"Key not found in hashmap");
}

ErrorResult fun_hashmap_contains(const HashMap *map, const void *key,
								 bool *out_contains)
{
	if (!map || !key || !out_contains) {
		return ERROR_RESULT_NULL_POINTER;
	}

	// Calculate hash and bucket index
	uint64_t hash = map->hash_fn(key);
	size_t bucket_idx = hashmap_bucket_index(map, hash);

	// Check if entry exists in chain
	HashMapEntry *entry = hashmap_find_entry_in_chain(map->buckets[bucket_idx],
													  key, map->equals_fn);

	*out_contains = (entry != NULL);
	return ERROR_RESULT_NO_ERROR;
}

size_t fun_hashmap_size(const HashMap *map)
{
	if (!map)
		return 0;
	return map->entry_count;
}

ErrorResult fun_hashmap_destroy(HashMap *map)
{
	if (!map) {
		return ERROR_RESULT_NULL_POINTER;
	}

	// Free all entries in all buckets
	for (size_t i = 0; i < map->bucket_count; i++) {
		HashMapEntry *current = map->buckets[i];
		while (current) {
			HashMapEntry *next = current->next;

			// Free key and value
			fun_memory_free((Memory *)&current->key);
			fun_memory_free((Memory *)&current->value);

			// Free entry
			fun_memory_free((Memory *)&current);

			current = next;
		}
	}

	// Free bucket array
	if (map->buckets) {
		fun_memory_free((Memory *)&map->buckets);
	}

	// Reset the HashMap
	map->buckets = NULL;
	map->bucket_count = 0;
	map->entry_count = 0;
	map->key_size = 0;
	map->value_size = 0;
	map->hash_fn = NULL;
	map->equals_fn = NULL;

	return ERROR_RESULT_NO_ERROR;
}
