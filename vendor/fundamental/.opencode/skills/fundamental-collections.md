---
name: fundamental-collections
description: Collections with Fundamental Library - arrays, hashmaps, sets, trees
license: MIT
compatibility: Complements fundamental-expert skill
metadata:
  author: fundamental-library
  version: "1.0"
  category: data-structures
  related: fundamental-memory
---

# Fundamental Library - Collections Skill

I provide copy-paste examples for using collections in the Fundamental Library.

---

## Quick Reference

| Collection | Type Definition | Operations |
|------------|----------------|------------|
| Dynamic Array | `DEFINE_ARRAY_TYPE(T)` | create, push, get, pop, destroy |
| HashMap | `StringIntHashMap`, `StringStringHashMap` | put, get, delete, iterate |
| Set | `IntSet`, `StringSet` | add, contains, remove |
| RB-Tree | `IntRBTree`, `StringRBTree` | insert, search, delete |

**See Also:** [fundamental-memory](fundamental-memory.md) for allocation

---

## Task: Define Dynamic Array Type

Create a type-safe dynamic array.

```c
#include "array/array.h"

// Define array type for integers
DEFINE_ARRAY_TYPE(int)

// Define array type for custom structs
typedef struct {
    int id;
    char name[64];
} Item;

DEFINE_ARRAY_TYPE(Item)
```

**Key Points:**
- Creates `IntArray`, `ItemArray` types
- Generates type-safe functions: `fun_array_int_*`, `fun_array_item_*`
- Must call corresponding destroy function to free memory

---

## Task: Use Dynamic Array

Create, populate, and iterate an array.

```c
#include "array/array.h"
#include "console/console.h"

DEFINE_ARRAY_TYPE(int)

void array_example(void)
{
    // Create array
    IntArrayResult array_result = fun_array_int_create(16);  // Initial capacity
    if (fun_error_is_error(array_result.error)) {
        return;
    }
    IntArray *array = &array_result.value;
    
    // Push elements
    fun_array_int_push(array, 10);
    fun_array_int_push(array, 20);
    fun_array_int_push(array, 30);
    
    // Get element
    int value = fun_array_int_get(array, 1);  // Returns 20
    
    // Get length
    size_t len = fun_array_int_length(array);  // Returns 3
    
    // Iterate
    for (size_t i = 0; i < len; i++) {
        int item = fun_array_int_get(array, i);
        // ... process item ...
    }
    
    // Pop (remove last)
    int last = fun_array_int_pop(array);
    
    // Destroy (REQUIRED - frees memory)
    fun_array_int_destroy(array);
}
```

**Key Points:**
- Always call `destroy()` when done
- Array grows automatically as you push
- Index bounds not checked - ensure valid indices

---

## Task: Use HashMap (String Keys, Int Values)

Store and retrieve key-value pairs.

```c
#include "hashmap/hashmap.h"
#include "console/console.h"

void hashmap_int_example(void)
{
    // Create hashmap
    StringIntHashMapResult hm_result = fun_hashmap_string_int_create(16);
    if (fun_error_is_error(hm_result.error)) {
        return;
    }
    StringIntHashMap *hm = &hm_result.value;
    
    // Put values
    fun_hashmap_string_int_put(hm, "port", 5432);
    fun_hashmap_string_int_put(hm, "timeout", 30);
    fun_hashmap_string_int_put(hm, "retries", 3);
    
    // Get value
    IntResult value_result = fun_hashmap_string_int_get(hm, "port");
    if (fun_error_is_error(value_result.error)) {
        // Key not found
    } else {
        int port = value_result.value;  // 5432
    }
    
    // Check if key exists
    BoolResult exists = fun_hashmap_string_int_contains(hm, "timeout");
    if (exists.value) {
        // Key exists
    }
    
    // Delete key
    fun_hashmap_string_int_delete(hm, "retries");
    
    // Destroy (REQUIRED)
    fun_hashmap_string_int_destroy(hm);
}
```

---

## Task: Use HashMap (String Keys, String Values)

Store string key-value pairs.

```c
#include "hashmap/hashmap.h"

void hashmap_string_example(void)
{
    StringStringHashMapResult hm_result = fun_hashmap_string_string_create(16);
    StringStringHashMap *hm = &hm_result.value;
    
    // Put string values
    fun_hashmap_string_string_put(hm, "host", "localhost");
    fun_hashmap_string_string_put(hm, "database", "myapp");
    
    // Get value
    StringResult value = fun_hashmap_string_string_get(hm, "host");
    if (fun_error_is_ok(value.error)) {
        const char *host = value.value;  // "localhost"
    }
    
    // Destroy
    fun_hashmap_string_string_destroy(hm);
}
```

**Key Points:**
- HashMap does NOT copy string keys/values
- Ensure strings outlive the hashmap (or use owned strings)

---

## Task: Iterate HashMap

Loop over all key-value pairs.

```c
#include "hashmap/hashmap.h"
#include "console/console.h"

void hashmap_iterate_example(void)
{
    StringIntHashMapResult hm_result = fun_hashmap_string_int_create(16);
    StringIntHashMap *hm = &hm_result.value;
    
    // Populate
    fun_hashmap_string_int_put(hm, "a", 1);
    fun_hashmap_string_int_put(hm, "b", 2);
    fun_hashmap_string_int_put(hm, "c", 3);
    
    // Get count
    size_t count = fun_hashmap_string_int_count(hm);
    
    // Iterate by index
    for (size_t i = 0; i < count; i++) {
        HashMapStringIntEntry entry = fun_hashmap_string_int_get_at(hm, i);
        
        fun_console_write("Key: ");
        fun_console_write_line(entry.key);
        // fun_console_write_line(entry.value);  // Print value
    }
    
    fun_hashmap_string_int_destroy(hm);
}
```

---

## Task: Use Set

Store unique values.

```c
#include "set/set.h"

void set_example(void)
{
    // Create set
    IntSetResult set_result = fun_set_int_create(16);
    IntSet *set = &set_result.value;
    
    // Add elements
    fun_set_int_add(set, 1);
    fun_set_int_add(set, 2);
    fun_set_int_add(set, 3);
    fun_set_int_add(set, 2);  // Duplicate - ignored
    
    // Check membership
    BoolResult contains = fun_set_int_contains(set, 2);
    if (contains.value) {
        // 2 is in the set
    }
    
    // Remove element
    fun_set_int_remove(set, 1);
    
    // Get count
    size_t count = fun_set_int_count(set);  // 2
    
    // Destroy
    fun_set_int_destroy(set);
}
```

**String Set:**
```c
StringSetResult set_result = fun_set_string_create(16);
fun_set_string_add(&set_result.value, "apple");
fun_set_string_add(&set_result.value, "banana");
fun_set_string_contains(&set_result.value, "apple");
fun_set_string_destroy(&set_result.value);
```

---

## Task: Use Red-Black Tree

Sorted key-value storage.

```c
#include "rbtree/rbtree.h"

void rbtree_example(void)
{
    // Create tree
    IntRBTreeResult tree_result = fun_rbtree_int_create();
    IntRBTree *tree = &tree_result.value;
    
    // Insert
    fun_rbtree_int_insert(tree, 50);
    fun_rbtree_int_insert(tree, 30);
    fun_rbtree_int_insert(tree, 70);
    fun_rbtree_int_insert(tree, 20);
    
    // Search
    BoolResult found = fun_rbtree_int_search(tree, 30);
    if (found.value) {
        // Found 30 in tree
    }
    
    // Delete
    fun_rbtree_int_delete(tree, 20);
    
    // Destroy
    fun_rbtree_int_destroy(tree);
}
```

**String RB-Tree:**
```c
StringRBTreeResult tree_result = fun_rbtree_string_create();
fun_rbtree_string_insert(&tree_result.value, "banana");
fun_rbtree_string_insert(&tree_result.value, "apple");
fun_rbtree_string_search(&tree_result.value, "apple");
fun_rbtree_string_destroy(&tree_result.value);
```

---

## Memory Management

**CRITICAL: Always destroy collections**

❌ **Memory leak:**
```c
IntArrayResult r = fun_array_int_create(16);
// ... use array ...
// Forgot to destroy - LEAK!
```

✅ **Correct:**
```c
IntArrayResult r = fun_array_int_create(16);
// ... use array ...
fun_array_int_destroy(&r.value);  // Always free
```

**Destroy on error paths too:**
```c
IntArrayResult r = fun_array_int_create(16);
if (some_error) {
    fun_array_int_destroy(&r.value);  // Free before return
    return 1;
}
// ... use array ...
fun_array_int_destroy(&r.value);
```

---

## See Also

- **[fundamental-memory.md](fundamental-memory.md)** - Memory allocation patterns
- **[array/array.h](../../include/array/array.h)** - Dynamic arrays API
- **[hashmap/hashmap.h](../../include/hashmap/hashmap.h)** - Hash maps API
- **[set/set.h](../../include/set/set.h)** - Sets API
- **[rbtree/rbtree.h](../../include/rbtree/rbtree.h)** - Red-black trees API
