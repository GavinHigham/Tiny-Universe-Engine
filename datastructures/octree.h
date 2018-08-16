//INLINE HEADER FILE

#ifndef OCTREE_H
#define OCTREE_H

#include <stdbool.h>

enum { OCTREE_NUM_CHILDREN = 4 };

/*
Gen, prune and drawlist take a depth-definining function, and a void * context.
The depth function determines from the tree node and the context pointer how many times
the node should be split. Gen calls this function at every depth until it reaches a point
where the tree is sufficiently generated. Gen also takes a "split" function, which is
called on the void * tile member to generate split tiles for each child. 
*/

typedef struct octree_node {
	int depth;
	void *data;
	struct octree_node *children[OCTREE_NUM_CHILDREN];
} octree_node;

//The visit function performs some operation on the octree_node and/or its data.
//It returns true if the node's children should also be visited.
typedef bool (*octree_visit_fn)(octree_node *, void *);
typedef void (*octree_free_fn)(void *);

octree_node * octree_new(void *tile, int depth);
void octree_free(octree_node *tree, octree_free_fn free_data);
void octree_preorder_visit(octree_node *tree, octree_visit_fn visit, void *context);
void octree_postorder_visit(octree_node *tree, octree_visit_fn visit, void *context);
void octree_node_add_children(octree_node *node, void *child_data[OCTREE_NUM_CHILDREN]);
bool octree_node_has_children(octree_node *tree);
void octree_node_set_childless(octree_node *tree);

#endif