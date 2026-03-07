#include <string.h>

#include "../../include/set/set.h"

HashSetResult fun_set_create(size_t value_size, size_t initial_bucket_count,
							 HashFunction hash_fn, KeyEqualFunction equals_fn)
{
	HashSetResult result = { .error = ERROR_RESULT_NO_ERROR };

	if (initial_bucket_count == 0) {
		initial_bucket_count = 16;
	}

	MemoryResult buckets_result =
		fun_memory_allocate(initial_bucket_count * sizeof(SetEntry *));
	if (fun_error_is_error(buckets_result.error)) {
		result.error = buckets_result.error;
		return result;
	}

	SetEntry **buckets = (SetEntry **)buckets_result.value;
	for (size_t i = 0; i < initial_bucket_count; i++) {
		buckets[i] = NULL;
	}

	result.value.buckets = buckets;
	result.value.bucket_count = initial_bucket_count;
	result.value.entry_count = 0;
	result.value.value_size = value_size;
	result.value.hash_fn = hash_fn;
	result.value.equals_fn = equals_fn;

	return result;
}

ErrorResult fun_set_add(HashSet *set, const void *value)
{
	if (!set || !value) {
		return ERROR_RESULT_NULL_POINTER;
	}

	// Calculate bucket index
	uint64_t hash = set->hash_fn(value);
	size_t bucket_idx = hash % set->bucket_count;

	// Check if value already exists
	SetEntry *current = set->buckets[bucket_idx];
	while (current) {
		if (set->equals_fn(current->value, value)) {
			return ERROR_RESULT_NO_ERROR; // Already exists, no error
		}
		current = current->next;
	}

	// Create new entry
	SetEntry *new_entry =
		(SetEntry *)fun_memory_allocate(sizeof(SetEntry)).value;
	if (!new_entry) {
		return fun_error_result(ERROR_CODE_ELEMENT_EXISTS,
								"Failed to allocate set entry");
	}

	new_entry->value = fun_memory_allocate(set->value_size).value;
	if (!new_entry->value) {
		fun_memory_free((Memory *)&new_entry);
		return fun_error_result(ERROR_CODE_ELEMENT_EXISTS,
								"Failed to allocate value");
	}

	memcpy(new_entry->value, value, set->value_size);
	new_entry->next = set->buckets[bucket_idx];
	set->buckets[bucket_idx] = new_entry;
	set->entry_count++;

	return ERROR_RESULT_NO_ERROR;
}

ErrorResult fun_set_contains(const HashSet *set, const void *value,
							 bool *out_contains)
{
	if (!set || !value || !out_contains) {
		return ERROR_RESULT_NULL_POINTER;
	}

	uint64_t hash = set->hash_fn(value);
	size_t bucket_idx = hash % set->bucket_count;

	SetEntry *current = set->buckets[bucket_idx];
	while (current) {
		if (set->equals_fn(current->value, value)) {
			*out_contains = true;
			return ERROR_RESULT_NO_ERROR;
		}
		current = current->next;
	}

	*out_contains = false;
	return ERROR_RESULT_NO_ERROR;
}

ErrorResult fun_set_remove(HashSet *set, const void *value)
{
	if (!set || !value) {
		return ERROR_RESULT_NULL_POINTER;
	}

	uint64_t hash = set->hash_fn(value);
	size_t bucket_idx = hash % set->bucket_count;

	SetEntry *current = set->buckets[bucket_idx];
	SetEntry *prev = NULL;

	while (current) {
		if (set->equals_fn(current->value, value)) {
			// Found it
			if (prev) {
				prev->next = current->next;
			} else {
				set->buckets[bucket_idx] = current->next;
			}

			fun_memory_free((Memory *)&current->value);
			fun_memory_free((Memory *)&current);
			set->entry_count--;

			return ERROR_RESULT_NO_ERROR;
		}
		prev = current;
		current = current->next;
	}

	return fun_error_result(ERROR_CODE_ELEMENT_NOT_FOUND, "Element not in set");
}

size_t fun_set_size(const HashSet *set)
{
	if (!set)
		return 0;
	return set->entry_count;
}

ErrorResult fun_set_destroy(HashSet *set)
{
	if (!set) {
		return ERROR_RESULT_NULL_POINTER;
	}

	// Free all entries
	for (size_t i = 0; i < set->bucket_count; i++) {
		SetEntry *current = set->buckets[i];
		while (current) {
			SetEntry *next = current->next;
			fun_memory_free((Memory *)&current->value);
			fun_memory_free((Memory *)&current);
			current = next;
		}
	}

	// Free buckets
	if (set->buckets) {
		fun_memory_free((Memory *)&set->buckets);
	}

	set->buckets = NULL;
	set->bucket_count = 0;
	set->entry_count = 0;

	return ERROR_RESULT_NO_ERROR;
}
