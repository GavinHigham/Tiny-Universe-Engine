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

typedef bool (*quadtree_visit_fn)(quadtree_node *, void *);

quadtree_node * quadtree_new(void *tile, int depth);
void quadtree_preorder_visit(quadtree_node *tree, quadtree_visit_fn visit, void *context);
void quadtree_postorder_visit(quadtree_node *tree, quadtree_visit_fn visit, void *context);
void quadtree_free(quadtree_node *tree, void (*free_data)(void *data), bool children_only);

#endif