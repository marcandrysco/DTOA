#include "avltree.h"

__attribute__((noreturn)) static void throw(const char *format, ...)
{
	va_list args;

	va_start(args, format);
	fprintf(stderr, format, args);
	va_end(args);

	abort();
}

/*
 * Node definitions and macros.
 *   @LEFT: The left child index.
 *   @RIGHT: The right child index.
 *   @CMP2NODE: Comparison to child index macro.
 *   @OTHERNODE: Retrieves the opposite child index macro.
 *   @NODEDIR: Child node index to direction macro.
 */

#define LEFT	0
#define RIGHT	1
#define CMP2NODE(cmp)	(cmp > 0 ? RIGHT : LEFT)
#define OTHERNODE(node)	(node == RIGHT ? LEFT : RIGHT)
#define NODEDIR(node)	(node == RIGHT ? 1 : -1)


/**
 * As iter structure.
 *   @tree: The tree.
 *   @iter: The iterator.
 */

struct asiter_t {
	struct avltree_t *tree;
	struct avltree_iter_t iter;
};


/*
 * local function declarations
 */

static int compare_nodekey(const void *key, const struct avltree_node_t *node, void *arg);
static int compare_nodenode(const struct avltree_node_t *n1, const struct avltree_node_t *n2, void *arg);

static struct avltree_inst_t *inst_cast(struct avltree_node_t *node);
static void inst_del(struct avltree_node_t *node, void *arg);

static void *asiter_next(struct asiter_t *info);
static void asiter_delete(struct asiter_t *info);

static struct avltree_node_t *rotate_single(struct avltree_node_t *node, uint8_t dir);
static struct avltree_node_t *rotate_double(struct avltree_node_t *node, uint8_t dir);

/*
 * local variables
 */

static struct iter_i iter_iface = { (iter_f)avltree_iter_next, free };
static struct iter_i iter_keys_iface = { (iter_f)avltree_iter_next_key, free };
static struct iter_i iter_refs_iface = { (iter_f)avltree_iter_next_ref, free };


/**
 * Create an empty AVL tree root.
 *   &returns: The empty root.
 */

struct avltree_root_t avltree_root_empty()
{
	return (struct avltree_root_t){ NULL };
}


/**
 * Obtain the first element from the root.
 *   @root: The root.
 *   &returns: The first node from the root or null.
 */

struct avltree_node_t *avltree_root_first(struct avltree_root_t *root)
{
	struct avltree_node_t *node = root->node;

	if(node == NULL)
		return NULL;

	while(node->child[LEFT] != NULL)
		node = node->child[LEFT];

	return node;
}

/**
 * Obtain the last element from the root.
 *   @root: The root.
 *   &returns: The last node from the root or null.
 */

struct avltree_node_t *avltree_root_last(struct avltree_root_t *root)
{
	struct avltree_node_t *node = root->node;

	if(node == NULL)
		return NULL;

	while(node->child[RIGHT] != NULL)
		node = node->child[RIGHT];

	return node;
}


/**
 * Obtain the left-most node from the given node.
 *   @node: The given node.
 *   &returns: The left-mode node or null if the given node is null.
 */

struct avltree_node_t *avltree_node_first(struct avltree_node_t *node)
{
	if(node == NULL)
		return NULL;

	while(node->child[LEFT] != NULL)
		node = node->child[LEFT];

	return node;
}

/**
 * Obtain the right-most node from the given node.
 *   @node: The given node.
 *   &returns: The right-mode node or null if the given node is null.
 */

struct avltree_node_t *avltree_node_last(struct avltree_node_t *node)
{
	if(node == NULL)
		return NULL;

	while(node->child[RIGHT] != NULL)
		node = node->child[RIGHT];

	return node;
}

/**
 * Retrieve the previous node.
 *   @node: The current node.
 *   &returns: The previous node or null.
 */

struct avltree_node_t *avltree_node_prev(struct avltree_node_t *node)
{
	if(node->child[LEFT] != NULL) {
		node = node->child[LEFT];

		while(node->child[RIGHT] != NULL)
			node = node->child[RIGHT];

		return node;
	}
	else {
		while(node->parent != NULL) {
			if(node->parent->child[RIGHT] == node)
				break;

			node = node->parent;
		}

		return node->parent;
	}
}

/**
 * Retrieve the next node.
 *   @node: The current node.
 *   &returns: The next node or null.
 */

struct avltree_node_t *avltree_node_next(struct avltree_node_t *node)
{
	if(node->child[RIGHT] != NULL) {
		node = node->child[RIGHT];

		while(node->child[LEFT] != NULL)
			node = node->child[LEFT];

		return node;
	}
	else {
		while(node->parent != NULL) {
			if(node->parent->child[LEFT] == node)
				break;

			node = node->parent;
		}

		return node->parent;
	}
}


/**
 * Look up an AVL tree node from the root.
 *   @root: The root node.
 *   @key: The sought key.
 *   @compare: The node-key comparison function.
 *   @arg: An argument passed to the comparison function.
 *   &returns: The node if found, 'NULL' if not found.
 */

struct avltree_node_t *avltree_node_lookup(struct avltree_node_t *root, const void *key, avltree_compare_nodekey_f compare, void *arg)
{
	int cmp;
	struct avltree_node_t *node = root;

	while(node != NULL) {
		cmp = compare(key, node, arg);
		if(cmp == 0)
			return node;
		else
			node = node->child[CMP2NODE(cmp)];
	}

	return NULL;
}

/**
 * Look up a nearby node from the AVL tree root.
 *   @root: The root node.
 *   @key: The sought key.
 *   @compare: The node-key comparison function.
 *   @arg: An argument passed to the comparison function.
 *   &returns: A nearby or exact node, null if the tree is empty.
 */

struct avltree_node_t *avltree_node_nearby(struct avltree_node_t *root, const void *key, avltree_compare_nodekey_f compare, void *arg)
{
	int cmp;
	struct avltree_node_t *node = root, *prev = NULL;

	while(node != NULL) {
		prev = node;

		cmp = compare(key, node, arg);
		if(cmp == 0)
			return node;
		else
			node = node->child[CMP2NODE(cmp)];
	}

	return prev;
}

/**
 * Look a node that is LUB given the key.
 *   @root: The root node.
 *   @key: The sought key.
 *   @compare: The node-key comparison function.
 *   @arg: An argument passed to the comparison function.
 *   &returns: A nearby or exact node, null if the tree is empty.
 */

struct avltree_node_t *avltree_node_atleast(struct avltree_node_t *root, const void *key, avltree_compare_nodekey_f compare, void *arg)
{
	int cmp = 0;
	struct avltree_node_t *node = root, *prev = NULL;

	while(node != NULL) {
		prev = node;

		cmp = compare(key, node, arg);
		if(cmp == 0)
			return node;
		else
			node = node->child[CMP2NODE(cmp)];
	}

	if(cmp > 0)
		return avltree_node_next(prev);
	else
		return prev;
}

/**
 * Look a node that is GLB given the key.
 *   @root: The root node.
 *   @key: The sought key.
 *   @compare: The node-key comparison function.
 *   @arg: An argument passed to the comparison function.
 *   &returns: A nearby or exact node, null if the tree is empty.
 */

struct avltree_node_t *avltree_node_atmost(struct avltree_node_t *root, const void *key, avltree_compare_nodekey_f compare, void *arg)
{
	int cmp = 0;
	struct avltree_node_t *node = root, *prev = NULL;

	while(node != NULL) {
		prev = node;

		cmp = compare(key, node, arg);
		if(cmp == 0)
			return node;
		else
			node = node->child[CMP2NODE(cmp)];
	}

	if(cmp < 0)
		return avltree_node_prev(prev);
	else
		return prev;
}


/**
 * Insert an AVL tree node from the root.
 *   @root: A pointer to the root node.
 *   @node: The node to insert.
 *   @compare: The node-node comparison function.
 *   @arg: An argument passed to the comparison function.
 */

void avltree_node_insert(struct avltree_node_t **root, struct avltree_node_t *node, avltree_compare_nodenode_f compare, void *arg)
{
	int cmp;
	short i;
	uint8_t dir[AVLTREE_MAX_HEIGHT];
	struct avltree_node_t *stack[AVLTREE_MAX_HEIGHT];

	*node = AVLTREE_NODE_INIT;

	if(*root == NULL) {
		*root = node;
		node->parent = NULL;

		return;
	}

	stack[0] = *root;

	for(i = 0; stack[i] != NULL && i < AVLTREE_MAX_HEIGHT; i++) {
		cmp = compare(node, stack[i], arg);
		if(cmp == 0)
			throw("Key already exists.");

		dir[i] = CMP2NODE(cmp);
		stack[i+1] = stack[i]->child[dir[i]];
	}

	i--;
	stack[i]->child[dir[i]] = node;
	stack[i]->balance += NODEDIR(dir[i]);
	node->parent = stack[i];

	if(stack[i]->child[OTHERNODE(dir[i])] != NULL)
		return;

	while(i-- > 0) {
		struct avltree_node_t *node;

		stack[i]->balance += NODEDIR(dir[i]);

		if(stack[i]->balance == 0)
			break;

		if((stack[i]->balance > -2) && (stack[i]->balance < 2))
			continue;

		if(dir[i+1] == CMP2NODE(stack[i]->balance))
			node = rotate_single(stack[i], OTHERNODE(CMP2NODE(stack[i]->balance)));
		else
			node = rotate_double(stack[i], OTHERNODE(CMP2NODE(stack[i]->balance)));

		if(i == 0)
			*root = node;
		else
			stack[i-1]->child[dir[i-1]] = node;
		
		break;
	}
}

/**
 * Remove an AVL tree node from the root. If not found, no node is removed.
 *   @root: A pointer to the root node.
 *   @key: The sought key.
 *   @compare: The node-key comparison function.
 *   &returns: The node if found, 'NULL' if not found.
 */

struct avltree_node_t *avltree_node_remove(struct avltree_node_t **root, const void *key, avltree_compare_nodekey_f compare, void *arg)
{
	int cmp;
	short i, ii;
	uint8_t dir[AVLTREE_MAX_HEIGHT];
	struct avltree_node_t *stack[AVLTREE_MAX_HEIGHT], *node, *retval;

	if(*root == NULL)
		return NULL;

	stack[0] = *root;

	for(i = 0; stack[i] != NULL && i < AVLTREE_MAX_HEIGHT; i++) {
		cmp = compare(key, stack[i], arg);
		if(cmp == 0)
			break;

		dir[i] = CMP2NODE(cmp);
		stack[i+1] = stack[i]->child[dir[i]];
	}
	if(stack[i] == NULL)
		return NULL;

	dir[i] = CMP2NODE(stack[i]->balance);

	ii = i;
	node = stack[i]->child[dir[i]];
	if(node != NULL) {
		while(node->child[OTHERNODE(dir[ii])] != NULL) {
			i++;
			stack[i] = node;
			dir[i] = OTHERNODE(dir[ii]);
			node = node->child[dir[i]];
		}

		stack[i]->child[dir[i]] = node->child[dir[ii]];
		if(node->child[dir[ii]] != NULL)
			node->child[dir[ii]]->parent = stack[i];

		i++;

		node->child[LEFT] = stack[ii]->child[LEFT];
		node->child[RIGHT] = stack[ii]->child[RIGHT];
		node->balance = stack[ii]->balance;

		if(node->child[LEFT] != NULL)
			node->child[LEFT]->parent = node;

		if(node->child[RIGHT] != NULL)
			node->child[RIGHT]->parent = node;
	}

	if(ii == 0) {
		*root = node;

		if(node != NULL)
			node->parent = NULL;
	}
	else {
		stack[ii-1]->child[dir[ii-1]] = node;

		if(node != NULL)
			node->parent = stack[ii-1];
	}

	retval = stack[ii];
	stack[ii] = node;

	while(i-- > 0) {
		stack[i]->balance -= NODEDIR(dir[i]);

		if((stack[i]->balance > 1) || (stack[i]->balance < -1)) {
			if(stack[i]->balance == -2 * stack[i]->child[CMP2NODE(stack[i]->balance/2)]->balance)
				node = rotate_double(stack[i], OTHERNODE(CMP2NODE(stack[i]->balance)));
			else
				node = rotate_single(stack[i], OTHERNODE(CMP2NODE(stack[i]->balance)));

			if(i == 0)
				*root = node;
			else
				stack[i-1]->child[dir[i-1]] = node;

			stack[i] = node;
		}

		if(stack[i]->balance != 0)
			break;
	}

	return retval;
}

/**
 * Clear all nodes under a root through a deletion callback.
 *   @root: The root node.
 *   @delete: The deletion callback.
 */

void avltree_node_clear(struct avltree_node_t *root, avltree_delete_node_f delete, void *arg)
{
	short i;
	struct avltree_node_t *stack[AVLTREE_MAX_HEIGHT];
	uint8_t stack_s[AVLTREE_MAX_HEIGHT];

	if(root == NULL)
		return;

	stack[0] = root;
	stack_s[0] = 0;

	for(i = 0; i >= 0; ) {
		switch(stack_s[i]) {
		case 0:
			stack_s[i]++;
			if(stack[i]->child[LEFT] != NULL) {
				stack[i+1] = stack[i]->child[LEFT];
				stack_s[i+1] = 0;
				i++;

				break;
			}

		case 1:
			stack_s[i]++;
			if(stack[i]->child[RIGHT] != NULL) {
				stack[i+1] = stack[i]->child[RIGHT];
				stack_s[i+1] = 0;
				i++;

				break;
			}

		case 2:
			delete(stack[i--], arg);
		}
	}
}


/**
 * Begin a blank node iterator.
 *   &returns: The iterator.
 */

struct avltree_iter_t avltree_node_iter_blank()
{
	return (struct avltree_iter_t){ .i = -2 };
}

/**
 * Create a new iterator on the tree.
 *   @root: The root node.
 *   &returs: The iterator.
 */

struct avltree_iter_t avltree_node_iter_begin(struct avltree_node_t *root)
{
	struct avltree_iter_t iter;

	if(root == NULL)
		return avltree_node_iter_blank(root);
	else {
		iter.i = 0;
		iter.stack[0] = root;
		iter.state[0] = 0;
	}

	return iter;
}

/**
 * Retrieve the previous node from an AVL tree iterator.
 *   @iter: The iterator.
 *   &returns: The previous node, 'NULL' if all nodes are exhausted.
 */

struct avltree_node_t *avltree_node_iter_prev(struct avltree_iter_t *iter)
{
	throw("stub");
}

/**
 * Retrieve the next node from an AVL tree iterator.
 *   @iter: The iterator.
 *   &returns: The next node, 'NULL' if all nodes are exhausted.
 */

struct avltree_node_t *avltree_node_iter_next(struct avltree_iter_t *iter)
{
	struct avltree_node_t **stack;
	uint8_t *state;

	if(iter->i < 0)
		return NULL;

	stack = iter->stack;
	state = iter->state;

	for(;;) {
		short i = iter->i;

		switch(state[i]) {
		case 0:
			state[i]++;
			if(stack[i]->child[LEFT] != NULL) {
				stack[i+1] = stack[i]->child[LEFT];
				state[i+1] = 0;
				iter->i++;

				break;
			}

		case 1:
			state[i]++;

			return stack[i];

		case 2:
			state[i]++;
			if(stack[i]->child[RIGHT] != NULL) {
				stack[i+1] = stack[i]->child[RIGHT];
				state[i+1] = 0;
				iter->i++;

				break;
			}

		case 3:
			if(--iter->i < 0)
				return NULL;
		}
	}
}

/**
 * Retrieve the next node from an AVL tree iterator using a depth-first
 * search.
 *   @iter: The iterator.
 *   &returns: The next node, 'NULL' if all nodes are exhausted.
 */

struct avltree_node_t *avltree_node_iter_next_depth(struct avltree_iter_t *iter)
{
	struct avltree_node_t **stack;
	uint8_t *state;

	if(iter->i < 0)
		return NULL;

	stack = iter->stack;
	state = iter->state;

	for(;;) {
		short i = iter->i;

		switch(state[i]) {
		case 0:
			state[i]++;
			if(stack[i]->child[LEFT] != NULL) {
				stack[i+1] = stack[i]->child[LEFT];
				state[i+1] = 0;
				iter->i++;

				break;
			}

		case 1:
			state[i]++;
			if(stack[i]->child[RIGHT] != NULL) {
				stack[i+1] = stack[i]->child[RIGHT];
				state[i+1] = 0;
				iter->i++;

				break;
			}

		case 2:
			state[i]++;

			return stack[i];

		case 3:
			if(--iter->i < 0)
				return NULL;
		}
	}
}


/**
 * Initialize the AVL tree.
 *   @tree: The uninitialized AVL tree.
 *   @compare: The comparison function used to sort reference.
 *   @delete: Optional. The callback to delete references. Set to 'NULL' if
 *     unused.
 */

void avltree_init(struct avltree_t *tree, compare_f compare, delete_f delete)
{
	*tree = avltree_empty(compare, delete);
}

/**
 * Create an empty AVL tree.
 *   @compare: The comparison function used to sort reference.
 *   @delete: Optional. The callback to delete references. Set to 'NULL' if
 *     unused.
 *   &returns: The empty tree.
 */

struct avltree_t avltree_empty(compare_f compare, delete_f delete)
{
	return (struct avltree_t){ avltree_root_empty(), 0, compare, delete };
}

/**
 * Allocates and initializes a new AVL tree.
 *   @compare: The comparison function used to sort reference.
 *   @delete: Optional. The callback to delete references. Set to 'NULL' if
 *     unused.
 *   &returns: The AVL tree.
 */

struct avltree_t *avltree_new(compare_f compare, delete_f delete)
{
	struct avltree_t *tree;

	tree = malloc(sizeof(struct avltree_t));
	avltree_init(tree, compare, delete);

	return tree;
}

/**
 * Cleans up all data associated with AVL tree and all its references.
 *   @tree: The AVL tree.
 */

void avltree_destroy(struct avltree_t *tree)
{
	avltree_clear(tree);
}

/**
 * Deletes the AVL tree an all its references.
 *   @tree: The AVL tree.
 */

void avltree_delete(struct avltree_t *tree)
{
	avltree_destroy(tree);

	free(tree);
}


/**
 * Obtain the first element in the tree.
 *   @tree: The AVL tree.
 *   &returns: The reference from the first element or null.
 */

void *avltree_first(struct avltree_t *tree)
{
	struct avltree_inst_t *inst;

	inst = avltree_inst_first(tree);
	return inst ? inst->ref : NULL;
}

/**
 * Obtain the last element in the tree.
 *   @tree: The AVL tree.
 *   &returns: The reference from the last element or null.
 */

void *avltree_last(struct avltree_t *tree)
{
	struct avltree_inst_t *inst;

	inst = avltree_inst_last(tree);
	return inst ? inst->ref : NULL;
}


/**
 * Lookup an AVL tree reference.
 *   @tree: The AVL tree.
 *   @key: The sought key.
 *   &returns: The reference if found, null otherwise.
 */

void *avltree_lookup(const struct avltree_t *tree, const void *key)
{
	struct avltree_node_t *node;

	node = avltree_node_lookup(tree->root.node, key, compare_nodekey, tree->compare);
	if(node == NULL)
		return NULL;

	return ((struct avltree_inst_t *)((void *)node - offsetof(struct avltree_inst_t, node)))->ref;
}

/**
 * Lookup a nearby AVL tree reference.
 *   @tree: The AVL tree.
 *   @key: The sought key.
 *   &returns: The reference if found, null otherwise.
 */

void *avltree_nearby(const struct avltree_t *tree, const void *key)
{
	struct avltree_node_t *node;

	node = avltree_node_nearby(tree->root.node, key, compare_nodekey, tree->compare);
	if(node == NULL)
		return NULL;

	return ((struct avltree_inst_t *)((void *)node - offsetof(struct avltree_inst_t, node)))->ref;
}

/**
 * Lookup the LUB node given the key.
 *   @tree: The AVL tree.
 *   @key: The sought key.
 *   &returns: The reference if found, null otherwise.
 */

void *avltree_atleast(const struct avltree_t *tree, const void *key)
{
	struct avltree_node_t *node;

	node = avltree_node_atleast(tree->root.node, key, compare_nodekey, tree->compare);
	if(node == NULL)
		return NULL;

	return ((struct avltree_inst_t *)((void *)node - offsetof(struct avltree_inst_t, node)))->ref;
}

/**
 * Lookup the GLB node given the key.
 *   @tree: The AVL tree.
 *   @key: The sought key.
 *   &returns: The reference if found, null otherwise.
 */

void *avltree_atmost(const struct avltree_t *tree, const void *key)
{
	struct avltree_node_t *node;

	node = avltree_node_atmost(tree->root.node, key, compare_nodekey, tree->compare);
	if(node == NULL)
		return NULL;

	return ((struct avltree_inst_t *)((void *)node - offsetof(struct avltree_inst_t, node)))->ref;
}


/**
 * Insert a key-reference pair into the AVL tree.
 *   @tree: The AVL tree.
 *   @key: The key reference.
 *   @ref: The value reference.
 */

void avltree_insert(struct avltree_t *tree, const void *key, void *ref)
{
	struct avltree_inst_t *value;

	value = malloc(sizeof(struct avltree_inst_t));
	value->key = key;
	value->ref = ref;

	avltree_node_insert(&tree->root.node, &value->node, compare_nodenode, tree->compare);
	tree->count++;
}

/**
 * Insert a reference as both the key and value.
 *   @tree: The tree.
 *   @ref: The reference.
 */

void avltree_insert_ref(struct avltree_t *tree, void *ref)
{
	avltree_insert(tree, ref, ref);
}

/**
 * Remove a reference from an AVL tree.
 *   @tree: The AVL tree.
 *   @key: The key reference.
 *   &returns: The value reference if found, 'NULL' otherwise.
 */

void *avltree_remove(struct avltree_t *tree, const void *key)
{
	struct avltree_node_t *node;
	struct avltree_inst_t *ref;
	void *value;

	node = avltree_node_remove(&tree->root.node, key, compare_nodekey, tree->compare);
	if(node == NULL)
		return node;

	ref = (void *)node - offsetof(struct avltree_inst_t, node);
	value = ref->ref;

	free(ref);
	tree->count--;

	return value;
}

/**
 * Removes and delete a reference from the AVL tree.
 *   @tree: The AVL tree.
 *   @key: The key reference.
 */

void avltree_purge(struct avltree_t *tree, const void *key)
{
	void *ref;

	ref = avltree_remove(tree, key);
	if(ref == NULL)
		throw("Key not found.");

	if(tree->delete != NULL)
		tree->delete(ref);
}


/**
 * Merge two trees together.
 *   @dest: The destination tree.
 *   @src: Consumed. The source tree.
 */
void avltree_merge(struct avltree_t *dest, struct avltree_t *src)
{
	struct avltree_iter_t iter;
	struct avltree_node_t *node;

	iter = avltree_node_iter_begin(src->root.node);
	while((node = avltree_node_iter_next_depth(&iter)) != NULL)
		avltree_node_insert(&dest->root.node, node, compare_nodenode, dest->compare);

	dest->count += src->count;
	src->root = avltree_root_empty();
	src->count = 0;
}

/**
 * Delete every reference from a tree, leaving it empty.
 *   @tree: The AVL tree.
 *   &prop: noerror
 */

void avltree_clear(struct avltree_t *tree)
{
	avltree_node_clear(tree->root.node, inst_del, tree->delete);

	tree->root = avltree_root_empty();
	tree->count = 0;
}


/**
 * Create a new iterator on the tree.
 *   @tree: The AVL tree.
 *   &returs: The iterator.
 */

struct avltree_iter_t avltree_iter(const struct avltree_t *tree)
{
	return avltree_node_iter_begin(tree->root.node);
}

/**
 * Begin a blank iterator.
 *   &returns: The iterator.
 */

struct avltree_iter_t avltree_iter_blank()
{
	return avltree_node_iter_blank();
}

/**
 * Create a new iterator on the tree.
 *   @tree: The AVL tree.
 *   &returs: The iterator.
 */

struct avltree_iter_t avltree_iter_begin(const struct avltree_t *tree)
{
	return avltree_node_iter_begin(tree->root.node);
}

/**
 * Initialize an iterator on the AVL tree.
 *   @iter: The uninitailized iterator structure.
 *   @tree: The AVL tree.
 */

void avltree_iter_init(struct avltree_iter_t *iter, const struct avltree_t *tree)
{
	*iter = avltree_node_iter_begin(tree->root.node);
}

/**
 * Retrieve the previous reference from an AVL tree iterator.
 *   @iter: The iterator.
 *   &returns: The previous reference, 'NULL' if all references are exhausted.
 */

void *avltree_iter_prev(struct avltree_iter_t *iter)
{
	struct avltree_node_t *node;

	node = avltree_node_iter_prev(iter);
	if(node == NULL)
		return NULL;

	return ((struct avltree_inst_t *)((void *)node - offsetof(struct avltree_inst_t, node)))->ref;
}

/**
 * Retrieve the next reference from an AVL tree iterator.
 *   @iter: The iterator.
 *   &returns: The next reference, 'NULL' if all references are exhausted.
 */

void *avltree_iter_next(struct avltree_iter_t *iter)
{
	struct avltree_node_t *node;

	node = avltree_node_iter_next(iter);
	if(node == NULL)
		return NULL;

	return ((struct avltree_inst_t *)((void *)node - offsetof(struct avltree_inst_t, node)))->ref;
}

/**
 * Retrieve the next key reference from an AVL tree iterator.
 *   @iter: The iterator.
 *   &returns: The next key reference, 'NULL' if all references are exhausted.
 */

void *avltree_iter_next_key(struct avltree_iter_t *iter)
{
	struct avltree_node_t *node;

	node = avltree_node_iter_next(iter);
	if(node == NULL)
		return NULL;

	return (void *)((struct avltree_inst_t *)((void *)node - offsetof(struct avltree_inst_t, node)))->key;
}

/**
 * Retrieve the next reference structure from an AVL tree iterator.
 *   @iter: The iterator.
 *   &returns: The next key reference, 'NULL' if all references are exhausted.
 */

struct avltree_inst_t *avltree_iter_next_ref(struct avltree_iter_t *iter)
{
	struct avltree_node_t *node;

	node = avltree_node_iter_next(iter);
	if(node == NULL)
		return NULL;

	return (void *)node - offsetof(struct avltree_inst_t, node);
}


/**
 * Create a new iterator over the references in an AVL tree.
 *   @tree: The AVL tree.
 *   &returns: The iterator.
 */

struct iter_t avltree_iter_new(const struct avltree_t *tree)
{
	struct iter_t iter;

	iter.ref = malloc(sizeof(struct avltree_iter_t));
	iter.iface = &iter_iface;

	avltree_iter_init(iter.ref, tree);

	return iter;
}

/**
 * Create a new iterator over the keys in an AVL tree.
 *   @tree: The AVL tree.
 *   &returns: The iterator.
 */

struct iter_t avltree_iter_keys_new(const struct avltree_t *tree)
{
	struct iter_t iter;

	iter.ref = malloc(sizeof(struct avltree_iter_t));
	iter.iface = &iter_keys_iface;

	avltree_iter_init(iter.ref, tree);

	return iter;
}

/**
 * Create a new iterator over the reference structures in an AVL tree.
 *   @tree: The AVL tree.
 *   &returns: The iterator.
 */

struct iter_t avltree_iter_refs_new(const struct avltree_t *tree)
{
	struct iter_t iter;

	iter.ref = malloc(sizeof(struct avltree_iter_t));
	iter.iface = &iter_refs_iface;

	avltree_iter_init(iter.ref, tree);

	return iter;
}


/**
 * Create a new iterator over the references, deleting the tree in the end.
 *   @tree: The AVL tree.
 *   &returns: The iterator.
 */

struct iter_t avltree_asiter(struct avltree_t *tree)
{
	struct asiter_t *info;
	static const struct iter_i iface = { (iter_f)asiter_next, (delete_f)asiter_delete };

	info = malloc(sizeof(struct asiter_t));
	info->tree = tree;
	info->iter = avltree_iter_begin(info->tree);

	return (struct iter_t){ info, &iface };
}

/**
 * Retrieve the next reference from the information structure.
 *   @info: The information structure.
 *   &returns: The reference or null.
 */

static void *asiter_next(struct asiter_t *info)
{
	return avltree_iter_next(&info->iter);
}

/**
 * Delete the asiter structure.
 *   @info: The info structure.
 */

static void asiter_delete(struct asiter_t *info)
{
	avltree_delete(info->tree);
	free(info);
}


/**
 * Execute an iteration callback on every reference within the tree.
 *   @tree: The AVL tree.
 *   @func: The callback function.
 *   @arg: An arugment passed to the callbcak.
 */

void avltree_iterate(const struct avltree_t *tree, avltree_iterate_f func, void *arg)
{
	struct avltree_iter_t iter;
	void *ref;

	avltree_iter_init(&iter, tree);
	while((ref = avltree_iter_next(&iter)) != NULL) {
		if(func(ref, arg))
			break;
	}
}

/**
 * Execute an iteration callback on every key within the tree.
 *   @tree: The AVL tree.
 *   @func: The callback function.
 *   @arg: An arugment passed to the callbcak.
 */

void avltree_iterate_keys(const struct avltree_t *tree, avltree_iterate_key_f func, void *arg)
{
	struct avltree_iter_t iter;
	const void *key;

	avltree_iter_init(&iter, tree);
	while((key = avltree_iter_next_key(&iter)) != NULL) {
		if(func(key, arg))
			break;
	}
}

/**
 * Execute an iteration callback on every reference structure within the tree.
 *   @tree: The AVL tree.
 *   @func: The callback function.
 *   @arg: An arugment passed to the callbcak.
 */

void avltree_iterate_refs(const struct avltree_t *tree, avltree_iterate_ref_f func, void *arg)
{
	struct avltree_iter_t iter;
	struct avltree_inst_t *ref;

	avltree_iter_init(&iter, tree);
	while((ref = avltree_iter_next_ref(&iter)) != NULL) {
		if(func(ref, arg))
			break;
	}
}


/**
 * Retrieve the first instance from the tree.
 *   @tree: The tree.
 *   &returns: The instance.
 */

struct avltree_inst_t *avltree_inst_first(struct avltree_t *tree)
{
	struct avltree_node_t *node;

	node = avltree_root_first(&tree->root);
	return node ? inst_cast(node) : NULL;
}

/**
 * Retrieve the last instance from the tree.
 *   @tree: The tree.
 *   &returns: The instance.
 */

struct avltree_inst_t *avltree_inst_last(struct avltree_t *tree)
{
	struct avltree_node_t *node;

	node = avltree_root_last(&tree->root);
	return node ? inst_cast(node) : NULL;
}

/**
 * Retrieve the previous instance.
 *   @inst: The current instance.
 *   &returns: The instance.
 */

struct avltree_inst_t *avltree_inst_prev(struct avltree_inst_t *inst)
{
	struct avltree_node_t *node;

	node = avltree_node_prev(&inst->node);
	return node ? inst_cast(node) : NULL;
}

/**
 * Retrieve the next instance.
 *   @inst: The current instance.
 *   &returns: The instance.
 */

struct avltree_inst_t *avltree_inst_next(struct avltree_inst_t *inst)
{
	struct avltree_node_t *node;

	node = avltree_node_next(&inst->node);
	return node ? inst_cast(node) : NULL;
}


/**
 * Lookup the LUB instance given the key.
 *   @tree: The AVL tree.
 *   @key: The sought key.
 *   &returns: The refernce if found, null otherwise.
 */

struct avltree_inst_t *avltree_inst_atleast(const struct avltree_t *tree, const void *key)
{
	struct avltree_node_t *node;

	node = avltree_node_atleast(tree->root.node, key, compare_nodekey, tree->compare);
	return node ? inst_cast(node) : NULL;
}

/**
 * Lookup the GLB instance given the key.
 *   @tree: The AVL tree.
 *   @key: The sought key.
 *   &returns: The refernce if found, null otherwise.
 */

struct avltree_inst_t *avltree_inst_atmost(const struct avltree_t *tree, const void *key)
{
	struct avltree_node_t *node;

	node = avltree_node_atmost(tree->root.node, key, compare_nodekey, tree->compare);
	return node ? inst_cast(node) : NULL;
}


/**
 * Compare a reference node against a key.
 *   @key: The key reference.
 *   @node: The node.
 *   @arg: The reference comparison.
 *   &returns: An integer representing their order.
 */

static int compare_nodekey(const void *key, const struct avltree_node_t *node, void *arg)
{
	compare_f compare = arg;
	struct avltree_inst_t *ref = (void *)node - offsetof(struct avltree_inst_t, node);

	return compare(key, ref->key);
}

/**
 * Compare two reference nodes against one another.
 *   @n1: The first node.
 *   @n2: The second node.
 *   @arg: The reference comparison.
 *   &returns: An integer representing their order.
 */

static int compare_nodenode(const struct avltree_node_t *n1, const struct avltree_node_t *n2, void *arg)
{
	compare_f compare = arg;
	struct avltree_inst_t *r1 = (void *)n1 - offsetof(struct avltree_inst_t, node);
	struct avltree_inst_t *r2 = (void *)n2 - offsetof(struct avltree_inst_t, node);

	return compare(r1->key, r2->key);
}


/**
 * Delete an instance while clearing.
 *   @node: The node.
 *   @arg: The deletion callback.
 */

static void inst_del(struct avltree_node_t *node, void *arg)
{
	delete_f delete = arg;
	struct avltree_inst_t *ref = (void *)node - offsetof(struct avltree_inst_t, node);

	if(delete != NULL)
		delete(ref->ref);

	free(ref);
}

/**
 * Cast a node to an instance.
 *   @node: The node.
 *   &returns: The instance.
 */

static struct avltree_inst_t *inst_cast(struct avltree_node_t *node)
{
	return (void *)node - offsetof(struct avltree_inst_t, node);
}


/**
 * Performs a single tree rotation of the given node. The node's child in the
 * opposite direction as the 'dir' paramter will replace itself as the parent,
 * placing the old parent as a child in the direction of the 'dir' parameter.
 * Wikipedia provides a good explanation with pictures.
 *   @node: The AVL tree node.
 *   @dir: The direction to rotate, should be either the value 'LEFT' or
 *     'RIGHT'.
 *   &returns: The node that now takes the place of the node that was passed
 *     in.
 */

static struct avltree_node_t *rotate_single(struct avltree_node_t *node, uint8_t dir)
{
	struct avltree_node_t *tmp;

	tmp = node->child[OTHERNODE(dir)];
	node->child[OTHERNODE(dir)] = tmp->child[dir];
	tmp->child[dir] = node;

	node->balance += NODEDIR(dir);
	if(NODEDIR(dir) * tmp->balance < 0)
		node->balance -= tmp->balance;

	tmp->balance += NODEDIR(dir);
	if(NODEDIR(dir) * node->balance > 0)
		tmp->balance += node->balance;

	tmp->parent = node->parent;
	node->parent = tmp;

	if(node->child[OTHERNODE(dir)] != NULL)
		node->child[OTHERNODE(dir)]->parent = node;

	return tmp;
}

/**
 * Performs a double rotation that is used to bring the grandchild to replace
 * its current position. Wikipedia provides a good explanation with pictures.
 *   @node: The AVL tree node.
 *   @dir: The direction to rotate, should be either the value 'LEFT' or
 *     'RIGHT'.
 *   &returns: The node that now takes the place of the node that was passed
 *     in.
 */

static struct avltree_node_t *rotate_double(struct avltree_node_t *node, uint8_t dir)
{
	node->child[OTHERNODE(dir)] = rotate_single(node->child[OTHERNODE(dir)], OTHERNODE(dir));

	return rotate_single(node, dir);
}
