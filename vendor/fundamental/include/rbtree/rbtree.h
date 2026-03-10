#ifndef LIBRARY_RBTREE_H
#define LIBRARY_RBTREE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "../collections/utils.h" // Reuse hash/equality utilities
#include "../error/error.h"
#include "../memory/memory.h"

// Red-Black Tree node colors
typedef enum { RB_RED = 0, RB_BLACK = 1 } RBColor;

// RBTree node structure
typedef struct RBNode {
  void *key;
  void *value; // For key-value storage (use NULL for set)
  struct RBNode *left;
  struct RBNode *right;
  struct RBNode *parent;
  RBColor color;
} RBNode;

// Core RBTree structure - type-agnostic
typedef struct {
  RBNode *root;
  size_t size;
  size_t key_size;
  size_t value_size; // 0 for set (key-only)
  HashFunction hash_fn;
  KeyEqualFunction equals_fn;
  RBNode *nil; // Sentinel node (NULL alternative)
} RBTree;

// Result type for RBTree operations
typedef struct {
  RBTree value;
  ErrorResult error;
} RBTreeResult;

// Error codes specific to RBTree
#define ERROR_CODE_KEY_EXISTS 301
#define ERROR_CODE_KEY_NOT_FOUND 302

// Core RBTree API - type-agnostic operations
RBTreeResult fun_rbtree_create(size_t key_size, size_t value_size,
                               HashFunction hash_fn,
                               KeyEqualFunction equals_fn);
ErrorResult fun_rbtree_insert(RBTree *tree, const void *key, const void *value);
ErrorResult fun_rbtree_get(const RBTree *tree, const void *key,
                           void *out_value);
ErrorResult fun_rbtree_remove(RBTree *tree, const void *key);
ErrorResult fun_rbtree_contains(const RBTree *tree, const void *key,
                                bool *out_contains);
size_t fun_rbtree_size(const RBTree *tree);
ErrorResult fun_rbtree_destroy(RBTree *tree);

// Macro to define type-safe RBTree with custom hash/equals functions
// Usage: DEFINE_RBTREE_TYPE_CUSTOM(Point, int, fun_hash_Point,
// fun_equals_Point)
#define DEFINE_RBTREE_TYPE_CUSTOM(K, V, HASH_FN, EQUALS_FN)                    \
  typedef struct {                                                             \
    RBTree tree;                                                               \
  } K##V##RBTree;                                                              \
                                                                               \
  typedef struct {                                                             \
    K##V##RBTree value;                                                        \
    ErrorResult error;                                                         \
  } K##V##RBTreeResult;                                                        \
                                                                               \
  static inline K##V##RBTreeResult fun_rbtree_##K##_##V##_create(void) {       \
    K##V##RBTreeResult result;                                                 \
    RBTreeResult tree_result =                                                 \
        fun_rbtree_create(sizeof(K), sizeof(V), HASH_FN, EQUALS_FN);           \
    result.error = tree_result.error;                                          \
    result.value.tree = tree_result.value;                                     \
    return result;                                                             \
  }                                                                            \
                                                                               \
  static inline ErrorResult fun_rbtree_##K##_##V##_insert(K##V##RBTree *tree,  \
                                                          K key, V value) {    \
    return fun_rbtree_insert(&tree->tree, &key, &value);                       \
  }                                                                            \
                                                                               \
  static inline V fun_rbtree_##K##_##V##_get(const K##V##RBTree *tree,         \
                                             K key) {                          \
    V value;                                                                   \
    ErrorResult result = fun_rbtree_get(&tree->tree, &key, &value);            \
    (void)result;                                                              \
    return value;                                                              \
  }                                                                            \
                                                                               \
  static inline ErrorResult fun_rbtree_##K##_##V##_remove(K##V##RBTree *tree,  \
                                                          K key) {             \
    return fun_rbtree_remove(&tree->tree, &key);                               \
  }                                                                            \
                                                                               \
  static inline ErrorResult fun_rbtree_##K##_##V##_contains(                   \
      const K##V##RBTree *tree, K key, bool *out) {                            \
    return fun_rbtree_contains(&tree->tree, &key, out);                        \
  }                                                                            \
                                                                               \
  static inline size_t fun_rbtree_##K##_##V##_size(const K##V##RBTree *tree) { \
    return fun_rbtree_size(&tree->tree);                                       \
  }                                                                            \
                                                                               \
  static inline ErrorResult fun_rbtree_##K##_##V##_destroy(                    \
      K##V##RBTree *tree) {                                                    \
    return fun_rbtree_destroy(&tree->tree);                                    \
  }

// Convenience macro for primitive types with existing hash/equals
// Usage: DEFINE_RBTREE_TYPE(int, int)
#define DEFINE_RBTREE_TYPE(K, V)                                               \
  DEFINE_RBTREE_TYPE_CUSTOM(K, V, fun_hash_##K, fun_equals_##K)

// Set variant (key-only, no value) - uses 0 for value_size
// Usage: DEFINE_RBSET_TYPE(Point, fun_hash_Point, fun_equals_Point)
#define DEFINE_RBSET_TYPE_CUSTOM(K, HASH_FN, EQUALS_FN)                        \
  typedef struct {                                                             \
    RBTree tree;                                                               \
  } K##Set;                                                                    \
                                                                               \
  typedef struct {                                                             \
    K##Set value;                                                              \
    ErrorResult error;                                                         \
  } K##SetResult;                                                              \
                                                                               \
  static inline K##SetResult fun_rbset_##K##_create(void) {                    \
    K##SetResult result;                                                       \
    RBTreeResult tree_result =                                                 \
        fun_rbtree_create(sizeof(K), 0, HASH_FN, EQUALS_FN);                   \
    result.error = tree_result.error;                                          \
    result.value.tree = tree_result.value;                                     \
    return result;                                                             \
  }                                                                            \
                                                                               \
  static inline ErrorResult fun_rbset_##K##_insert(K##Set *set, K key) {       \
    return fun_rbtree_insert(&set->tree, &key, NULL);                          \
  }                                                                            \
                                                                               \
  static inline ErrorResult fun_rbset_##K##_contains(const K##Set *set, K key, \
                                                     bool *out) {              \
    return fun_rbtree_contains(&set->tree, &key, out);                         \
  }                                                                            \
                                                                               \
  static inline ErrorResult fun_rbset_##K##_remove(K##Set *set, K key) {       \
    return fun_rbtree_remove(&set->tree, &key);                                \
  }                                                                            \
                                                                               \
  static inline size_t fun_rbset_##K##_size(const K##Set *set) {               \
    return fun_rbtree_size(&set->tree);                                        \
  }                                                                            \
                                                                               \
  static inline ErrorResult fun_rbset_##K##_destroy(K##Set *set) {             \
    return fun_rbtree_destroy(&set->tree);                                     \
  }

// Convenience for primitive type sets
#define DEFINE_RBSET_TYPE(K)                                                   \
  DEFINE_RBSET_TYPE_CUSTOM(K, fun_hash_##K, fun_equals_##K)

#endif // LIBRARY_RBTREE_H
