//INLINE HEADER FILE

#ifndef DYNAMIC_TERRAIN_TREE_H
#define DYNAMIC_TERRAIN_TREE_H

#include <stdbool.h>

enum { QUADTREE_NUM_CHILDREN = 4 };

/*
Gen, prune and drawlist take a depth-definining function, and a void * context.
The depth function determines from the tree node and the context pointer how many times
the node should be split. Gen calls this function at every depth until it reaches a point
where the tree is sufficiently generated. Gen also takes a "split" function, which is
called on the void * tile member to generate split tiles for each child. 
*/

typedef struct quadtree_node {
	int depth;
	void *data;
	struct quadtree_node *children[QUADTREE_NUM_CHILDREN];
} quadtree_node;

//The visit function performs some operation on the quadtree_node and/or its data.
//It returns true if the node's children should also be visited.
typedef bool (*quadtree_visit_fn)(quadtree_node *, void *);

quadtree_node * quadtree_new(void *tile, int depth);
void quadtree_free(quadtree_node *tree, void (*free_data)(void *data));
void quadtree_preorder_visit(quadtree_node *tree, quadtree_visit_fn visit, void *context);
void quadtree_postorder_visit(quadtree_node *tree, quadtree_visit_fn visit, void *context);
void quadtree_node_add_children(quadtree_node *node, void *child_data[QUADTREE_NUM_CHILDREN]);
bool quadtree_node_has_children(quadtree_node *tree);
void quadtree_node_set_childless(quadtree_node *tree);

#endif