#ifndef AVLTREE_H
#define AVLTREE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

/**
 * Initializer function.
 *   @ref: The reference.
 */

typedef void (*init_f)(void *ref);

/**
 * Delete a reference.
 *   @ref: The reference.
 */

typedef void (*delete_f)(void *ref);

/**
 * Copy a reference.
 *   @ref: The source reference.
 *   &returns: The copied reference.
 */

typedef void *(*copy_f)(void *ref);


/**
 * Value comparison.
 *   @p1: The first pointer value.
 *   @p2: The second pointer value.
 *   &returns: Their order.
 */

typedef int (*compare_f)(const void *p1, const void *p2);


/**
 * Obtain the next iterator reference.
 *   @ref: The iterator reference.
 *   &returns: The reference. 'NULL' if at the end of the iterator.
 */

typedef void *(*iter_f)(void *ref);

/**
 * Iterator interface structure.
 *   @next: The next function.
 *   @delete: The internal deletion function.
 */

struct iter_i {
	iter_f next;
	delete_f _delete;
};

/**
 * Iterator storage structure.
 *   @ref: The internal reference.
 *   @iface: The iterator interface.
 */

struct iter_t {
	void *ref;
	const struct iter_i *iface;
};

/**
 * AVL tree definitions.
 *   @AVLTREE_MAX_HEIGHT: The maximum height of an AVL tree.
 *   @AVLTREE_NODE_INIT: An inline structure to initialize a node.
 */

#define AVLTREE_MAX_HEIGHT	48
#define AVLTREE_NODE_INIT	(struct avltree_node_t){ 0, NULL, { NULL, NULL } }


/*
 * structure prototypes
 */

struct avltree_t;
struct avltree_node_t;
struct avltree_inst_t;
struct avltree_iter_t;


/**
 * AVL tree root structure.
 *   @node: The root node.
 */

struct avltree_root_t {
	struct avltree_node_t *node;
};

/**
 * AVL tree storage.
 *   @root: The root.
 *   @count: The number of nodes.
 *   @compare: The comparison function.
 *   @delete: The value deletion function.
 */

struct avltree_t {
	struct avltree_root_t root;

	unsigned int count;

	compare_f compare;
	delete_f _delete;
};

/**
 * AVL tree node storage.
 *   @balance: The current balance of the node, between '-2' to '2'.
 *   @parent, child: The parent and child nodes.
 */

struct avltree_node_t {
	int8_t balance;
	struct avltree_node_t *parent, *child[2];
};

/**
 * AVL tree reference storage.
 *   @key: The key reference.
 *   @ref: The value reference.
 *   @node: The node.
 */

struct avltree_inst_t {
	const void *key;
	void *ref;

	struct avltree_node_t node;
};

/**
 * AVL tree iterator storage.
 *   @i: The current level.
 *   @stack: The stack of nodes.
 *   @state: The stack of states per level.
 */

struct avltree_iter_t {
	short i;
	struct avltree_node_t *stack[AVLTREE_MAX_HEIGHT];
	uint8_t state[AVLTREE_MAX_HEIGHT];
};

/**
 * Comparison callback between a node and a key.
 *   @key: The input key.
 *   @node: The compared-to node.
 *   @arg: The callback argument.
 *   &returns: An integer representing their order.
 */

typedef int (*avltree_compare_nodekey_f)(const void *key, const struct avltree_node_t *node, void *arg);

/**
 * Comparison callback between two nodes.
 *   @n1: The first node.
 *   @n2: The seconde node.
 *   @arg: The callback argument.
 *   &returns: An integer representing their order.
 */

typedef int (*avltree_compare_nodenode_f)(const struct avltree_node_t *n1, const struct avltree_node_t *n2, void *arg);

/**
 * Deletion callback for a node.
 *   @node: The current node.
 *   @arg: The callback argument.
 */

typedef void (*avltree_delete_node_f)(struct avltree_node_t *node, void *arg);


/**
 * Iteration callback function on references.
 *   @ref: The reference.
 *   @arg: A user-specified argument.
 *   &returns: Non-zero to halt iteration, zero to continue.
 */

typedef short (*avltree_iterate_f)(void *ref, void *arg);

/**
 * Iteration callbcak function on keys.
 *   @ref: The reference.
 *   @arg: A user-specified argument.
 *   &returns: Non-zero to halt iteration, zero to continue.
 */

typedef short (*avltree_iterate_key_f)(const void *key, void *arg);

/**
 * Iteration callbcak function on reference structures.
 *   @ref: The reference.
 *   @arg: A user-specified argument.
 *   &returns: Non-zero to halt iteration, zero to continue.
 */

typedef short (*avltree_iterate_ref_f)(struct avltree_inst_t *ref, void *arg);


/*
 * avl tree node function declarations
 */

struct avltree_root_t avltree_root_empty();

struct avltree_node_t *avltree_root_first(struct avltree_root_t *root);
struct avltree_node_t *avltree_root_last(struct avltree_root_t *root);

struct avltree_node_t *avltree_node_first(struct avltree_node_t *node);
struct avltree_node_t *avltree_node_last(struct avltree_node_t *node);
struct avltree_node_t *avltree_node_prev(struct avltree_node_t *node);
struct avltree_node_t *avltree_node_next(struct avltree_node_t *node);

struct avltree_node_t *avltree_node_lookup(struct avltree_node_t *root, const void *key, avltree_compare_nodekey_f compare, void *arg);
struct avltree_node_t *avltree_node_nearby(struct avltree_node_t *root, const void *key, avltree_compare_nodekey_f compare, void *arg);
struct avltree_node_t *avltree_node_atleast(struct avltree_node_t *root, const void *key, avltree_compare_nodekey_f compare, void *arg);
struct avltree_node_t *avltree_node_atmost(struct avltree_node_t *root, const void *key, avltree_compare_nodekey_f compare, void *arg);

void avltree_node_insert(struct avltree_node_t **root, struct avltree_node_t *node, avltree_compare_nodenode_f compare, void *arg);
struct avltree_node_t *avltree_node_remove(struct avltree_node_t **root, const void *key, avltree_compare_nodekey_f compare, void *arg);
void avltree_node_clear(struct avltree_node_t *root, avltree_delete_node_f _delete, void *arg);

struct avltree_iter_t avltree_node_iter_blank();
struct avltree_iter_t avltree_node_iter_begin(struct avltree_node_t *root);
struct avltree_node_t *avltree_node_iter_prev(struct avltree_iter_t *iter);
struct avltree_node_t *avltree_node_iter_next(struct avltree_iter_t *iter);
struct avltree_node_t *avltree_node_iter_next_depth(struct avltree_iter_t *iter);

/*
 * avl tree function declarations
 */

void avltree_init(struct avltree_t *tree, compare_f compare, delete_f _delete);
struct avltree_t avltree_empty(compare_f compare, delete_f _delete);
struct avltree_t *avltree_new(compare_f compare, delete_f _delete);
void avltree_destroy(struct avltree_t *tree);
void avltree_delete(struct avltree_t *tree);

void *avltree_first(struct avltree_t *tree);
void *avltree_last(struct avltree_t *tree);

void *avltree_lookup(const struct avltree_t *tree, const void *key);
void *avltree_nearby(const struct avltree_t *tree, const void *key);
void *avltree_atleast(const struct avltree_t *tree, const void *key);
void *avltree_atmost(const struct avltree_t *tree, const void *key);

void avltree_insert(struct avltree_t *tree, const void *key, void *ref);
void avltree_insert_ref(struct avltree_t *tree, void *ref);
void *avltree_remove(struct avltree_t *tree, const void *key);
void avltree_purge(struct avltree_t *tree, const void *key);

void avltree_merge(struct avltree_t *dest, struct avltree_t *src);
void avltree_clear(struct avltree_t *tree);

struct avltree_iter_t avltree_iter(const struct avltree_t *tree);
struct avltree_iter_t avltree_iter_blank();
struct avltree_iter_t avltree_iter_begin(const struct avltree_t *tree);
void avltree_iter_init(struct avltree_iter_t *iter, const struct avltree_t *tree);
void *avltree_iter_prev(struct avltree_iter_t *iter);
void *avltree_iter_next(struct avltree_iter_t *iter);
void *avltree_iter_next_key(struct avltree_iter_t *iter);
struct avltree_inst_t *avltree_iter_next_ref(struct avltree_iter_t *iter);

struct iter_t avltree_iter_new(const struct avltree_t *tree);
struct iter_t avltree_iter_keys_new(const struct avltree_t *tree);
struct iter_t avltree_iter_refs_new(const struct avltree_t *tree);

struct iter_t avltree_asiter(struct avltree_t *tree);

void avltree_iterate(const struct avltree_t *tree, avltree_iterate_f func, void *arg);
void avltree_iterate_keys(const struct avltree_t *tree, avltree_iterate_key_f func, void *arg);
void avltree_iterate_refs(const struct avltree_t *tree, avltree_iterate_ref_f func, void *arg);

/*
 * avl tree instance function declarations
 */

struct avltree_inst_t *avltree_inst_first(struct avltree_t *tree);
struct avltree_inst_t *avltree_inst_last(struct avltree_t *tree);
struct avltree_inst_t *avltree_inst_prev(struct avltree_inst_t *inst);
struct avltree_inst_t *avltree_inst_next(struct avltree_inst_t *inst);

struct avltree_inst_t *avltree_inst_atleast(const struct avltree_t *tree, const void *key);
struct avltree_inst_t *avltree_inst_atmost(const struct avltree_t *tree, const void *key);

#ifdef __cplusplus
}
#endif

#endif
