#include "../../include/rbtree/rbtree.h"

// Internal: Create a new RB node
static RBNode *rbtree_create_node(const RBTree *tree, const void *key,
								  const void *value)
{
	RBNode *node = (RBNode *)fun_memory_allocate(sizeof(RBNode)).value;
	if (!node)
		return NULL;

	node->key = fun_memory_allocate(tree->key_size).value;
	if (!node->key) {
		fun_memory_free((Memory *)&node);
		return NULL;
	}
	fun_memory_copy((Memory)key, node->key, tree->key_size);

	if (tree->value_size > 0) {
		node->value = fun_memory_allocate(tree->value_size).value;
		if (!node->value) {
			fun_memory_free((Memory *)&node->key);
			fun_memory_free((Memory *)&node);
			return NULL;
		}
		fun_memory_copy((Memory)value, node->value, tree->value_size);
	} else {
		node->value = NULL;
	}

	node->left = tree->nil;
	node->right = tree->nil;
	node->parent = tree->nil;
	node->color = RB_RED;

	return node;
}

// Internal: Left rotation
static void rbtree_rotate_left(RBTree *tree, RBNode *node)
{
	RBNode *right = node->right;
	node->right = right->left;

	if (right->left != tree->nil) {
		right->left->parent = node;
	}

	right->parent = node->parent;

	if (node->parent == tree->nil) {
		tree->root = right;
	} else if (node == node->parent->left) {
		node->parent->left = right;
	} else {
		node->parent->right = right;
	}

	right->left = node;
	node->parent = right;
}

// Internal: Right rotation
static void rbtree_rotate_right(RBTree *tree, RBNode *node)
{
	RBNode *left = node->left;
	node->left = left->right;

	if (left->right != tree->nil) {
		left->right->parent = node;
	}

	left->parent = node->parent;

	if (node->parent == tree->nil) {
		tree->root = left;
	} else if (node == node->parent->right) {
		node->parent->right = left;
	} else {
		node->parent->left = left;
	}

	left->right = node;
	node->parent = left;
}

// Internal: Fixup after insert
static void rbtree_insert_fixup(RBTree *tree, RBNode *node)
{
	while (node->parent->color == RB_RED) {
		if (node->parent == node->parent->parent->left) {
			RBNode *uncle = node->parent->parent->right;

			if (uncle->color == RB_RED) {
				node->parent->color = RB_BLACK;
				uncle->color = RB_BLACK;
				node->parent->parent->color = RB_RED;
				node = node->parent->parent;
			} else {
				if (node == node->parent->right) {
					node = node->parent;
					rbtree_rotate_left(tree, node);
				}
				node->parent->color = RB_BLACK;
				node->parent->parent->color = RB_RED;
				rbtree_rotate_right(tree, node->parent->parent);
			}
		} else {
			RBNode *uncle = node->parent->parent->left;

			if (uncle->color == RB_RED) {
				node->parent->color = RB_BLACK;
				uncle->color = RB_BLACK;
				node->parent->parent->color = RB_RED;
				node = node->parent->parent;
			} else {
				if (node == node->parent->left) {
					node = node->parent;
					rbtree_rotate_right(tree, node);
				}
				node->parent->color = RB_BLACK;
				node->parent->parent->color = RB_RED;
				rbtree_rotate_left(tree, node->parent->parent);
			}
		}
	}

	tree->root->color = RB_BLACK;
}

// Internal: Find node by key
static RBNode *rbtree_find_node(const RBTree *tree, const void *key)
{
	RBNode *current = tree->root;

	while (current != tree->nil) {
		if (tree->equals_fn(current->key, key)) {
			return current;
		} else if (tree->hash_fn(current->key) < tree->hash_fn(key)) {
			current = current->right;
		} else {
			current = current->left;
		}
	}

	return NULL;
}

// Core API Implementation
RBTreeResult fun_rbtree_create(size_t key_size, size_t value_size,
							   HashFunction hash_fn, KeyEqualFunction equals_fn)
{
	RBTreeResult result = { .error = ERROR_RESULT_NO_ERROR };

	// Create sentinel nil node
	RBNode *nil = (RBNode *)fun_memory_allocate(sizeof(RBNode)).value;
	if (!nil) {
		result.error = fun_error_result(ERROR_CODE_KEY_NOT_FOUND,
										"Failed to allocate sentinel node");
		return result;
	}

	nil->left = NULL;
	nil->right = NULL;
	nil->parent = NULL;
	nil->color = RB_BLACK;
	nil->key = NULL;
	nil->value = NULL;

	result.value.root = nil;
	result.value.size = 0;
	result.value.key_size = key_size;
	result.value.value_size = value_size;
	result.value.hash_fn = hash_fn;
	result.value.equals_fn = equals_fn;
	result.value.nil = nil;

	return result;
}

ErrorResult fun_rbtree_insert(RBTree *tree, const void *key, const void *value)
{
	if (!tree || !key) {
		return ERROR_RESULT_NULL_POINTER;
	}

	// Check if key already exists
	if (rbtree_find_node(tree, key)) {
		// Update existing value
		RBNode *existing = rbtree_find_node(tree, key);
		if (tree->value_size > 0 && value) {
			fun_memory_copy((Memory)value, existing->value, tree->value_size);
		}
		return ERROR_RESULT_NO_ERROR;
	}

	// Create new node
	RBNode *new_node = rbtree_create_node(tree, key, value);
	if (!new_node) {
		return fun_error_result(ERROR_CODE_KEY_NOT_FOUND,
								"Failed to create new node");
	}

	// Standard BST insert
	RBNode *y = tree->nil;
	RBNode *x = tree->root;

	while (x != tree->nil) {
		y = x;
		if (tree->equals_fn(new_node->key, x->key)) {
			// Key exists, shouldn't happen due to check above
			fun_memory_free((Memory *)&new_node->key);
			if (new_node->value) {
				fun_memory_free((Memory *)&new_node->value);
			}
			fun_memory_free((Memory *)&new_node);
			return fun_error_result(ERROR_CODE_KEY_EXISTS,
									"Key already exists");
		} else if (tree->hash_fn(new_node->key) < tree->hash_fn(x->key)) {
			x = x->left;
		} else {
			x = x->right;
		}
	}

	new_node->parent = y;
	if (y == tree->nil) {
		tree->root = new_node;
	} else if (tree->hash_fn(new_node->key) < tree->hash_fn(y->key)) {
		y->left = new_node;
	} else {
		y->right = new_node;
	}

	new_node->left = tree->nil;
	new_node->right = tree->nil;
	new_node->color = RB_RED;

	tree->size++;

	// Fixup to maintain RB tree properties
	rbtree_insert_fixup(tree, new_node);

	return ERROR_RESULT_NO_ERROR;
}

ErrorResult fun_rbtree_get(const RBTree *tree, const void *key, void *out_value)
{
	if (!tree || !key || !out_value) {
		return ERROR_RESULT_NULL_POINTER;
	}

	RBNode *node = rbtree_find_node(tree, key);
	if (!node) {
		return fun_error_result(ERROR_CODE_KEY_NOT_FOUND,
								"Key not found in tree");
	}

	if (tree->value_size > 0) {
		fun_memory_copy(node->value, out_value, tree->value_size);
	}

	return ERROR_RESULT_NO_ERROR;
}

// Internal: Transplant nodes
static void rbtree_transplant(RBTree *tree, RBNode *u, RBNode *v)
{
	if (u->parent == tree->nil) {
		tree->root = v;
	} else if (u == u->parent->left) {
		u->parent->left = v;
	} else {
		u->parent->right = v;
	}
	v->parent = u->parent;
}

// Internal: Find minimum node
static RBNode *rbtree_minimum(RBTree *tree, RBNode *node)
{
	while (node->left != tree->nil) {
		node = node->left;
	}
	return node;
}

// Internal: Delete fixup
static void rbtree_delete_fixup(RBTree *tree, RBNode *node)
{
	while (node != tree->root && node->color == RB_BLACK) {
		if (node == node->parent->left) {
			RBNode *sibling = node->parent->right;

			if (sibling->color == RB_RED) {
				sibling->color = RB_BLACK;
				node->parent->color = RB_RED;
				rbtree_rotate_left(tree, node->parent);
				sibling = node->parent->right;
			}

			if (sibling->left->color == RB_BLACK &&
				sibling->right->color == RB_BLACK) {
				sibling->color = RB_RED;
				node = node->parent;
			} else {
				if (sibling->right->color == RB_BLACK) {
					sibling->left->color = RB_BLACK;
					sibling->color = RB_RED;
					rbtree_rotate_right(tree, sibling);
					sibling = node->parent->right;
				}

				sibling->color = node->parent->color;
				node->parent->color = RB_BLACK;
				sibling->right->color = RB_BLACK;
				rbtree_rotate_left(tree, node->parent);
				node = tree->root;
			}
		} else {
			// Mirror case
			RBNode *sibling = node->parent->left;

			if (sibling->color == RB_RED) {
				sibling->color = RB_BLACK;
				node->parent->color = RB_RED;
				rbtree_rotate_right(tree, node->parent);
				sibling = node->parent->left;
			}

			if (sibling->right->color == RB_BLACK &&
				sibling->left->color == RB_BLACK) {
				sibling->color = RB_RED;
				node = node->parent;
			} else {
				if (sibling->left->color == RB_BLACK) {
					sibling->right->color = RB_BLACK;
					sibling->color = RB_RED;
					rbtree_rotate_left(tree, sibling);
					sibling = node->parent->left;
				}

				sibling->color = node->parent->color;
				node->parent->color = RB_BLACK;
				sibling->left->color = RB_BLACK;
				rbtree_rotate_right(tree, node->parent);
				node = tree->root;
			}
		}
	}

	node->color = RB_BLACK;
}

ErrorResult fun_rbtree_remove(RBTree *tree, const void *key)
{
	if (!tree || !key) {
		return ERROR_RESULT_NULL_POINTER;
	}

	RBNode *node = rbtree_find_node(tree, key);
	if (!node) {
		return fun_error_result(ERROR_CODE_KEY_NOT_FOUND,
								"Key not found in tree");
	}

	RBNode *y = node;
	RBColor original_color = y->color;
	RBNode *x;

	if (node->left == tree->nil) {
		x = node->right;
		rbtree_transplant(tree, node, node->right);
	} else if (node->right == tree->nil) {
		x = node->left;
		rbtree_transplant(tree, node, node->left);
	} else {
		y = rbtree_minimum(tree, node->right);
		original_color = y->color;
		x = y->right;

		if (y->parent == node) {
			x->parent = y;
		} else {
			rbtree_transplant(tree, y, y->right);
			y->right = node->right;
			y->right->parent = y;
		}

		rbtree_transplant(tree, node, y);
		y->left = node->left;
		y->left->parent = y;
		y->color = node->color;
	}

	if (original_color == RB_BLACK) {
		rbtree_delete_fixup(tree, x);
	}

	// Free the removed node
	fun_memory_free((Memory *)&node->key);
	if (node->value) {
		fun_memory_free((Memory *)&node->value);
	}
	fun_memory_free((Memory *)&node);

	tree->size--;

	return ERROR_RESULT_NO_ERROR;
}

ErrorResult fun_rbtree_contains(const RBTree *tree, const void *key,
								bool *out_contains)
{
	if (!tree || !key || !out_contains) {
		return ERROR_RESULT_NULL_POINTER;
	}

	*out_contains = (rbtree_find_node(tree, key) != NULL);
	return ERROR_RESULT_NO_ERROR;
}

size_t fun_rbtree_size(const RBTree *tree)
{
	if (!tree)
		return 0;
	return tree->size;
}

// Internal: Destroy subtree recursively
static void rbtree_destroy_subtree(RBTree *tree, RBNode *node)
{
	if (node == tree->nil || node == NULL) {
		return;
	}

	rbtree_destroy_subtree(tree, node->left);
	rbtree_destroy_subtree(tree, node->right);

	fun_memory_free((Memory *)&node->key);
	if (node->value) {
		fun_memory_free((Memory *)&node->value);
	}
	fun_memory_free((Memory *)&node);
}

ErrorResult fun_rbtree_destroy(RBTree *tree)
{
	if (!tree) {
		return ERROR_RESULT_NULL_POINTER;
	}

	// Destroy all nodes in the tree
	rbtree_destroy_subtree(tree, tree->root);

	// Destroy sentinel node
	if (tree->nil) {
		fun_memory_free((Memory *)&tree->nil);
	}

	tree->root = NULL;
	tree->nil = NULL;
	tree->size = 0;

	return ERROR_RESULT_NO_ERROR;
}
