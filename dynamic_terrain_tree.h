//INLINE HEADER FILE

#ifndef DYNAMIC_TERRAIN_TREE_H
#define DYNAMIC_TERRAIN_TREE_H

#include <stdlib.h>
#include <stdbool.h>
#include <gl/glew.h>
#include "glla.h"
#include "math/space_sector.h"
#include "buffer_group.h"

enum { TERRAIN_TREE_NUM_CHILDREN = 4 };

/*
Gen, prune and drawlist take a depth-definining function, and a void * context.
The depth function determines from the tree node and the context pointer how many times
the node should be split. Gen calls this function at every depth until it reaches a point
where the tree is sufficiently generated. Gen also takes a "split" function, which is
called on the void * tile member to generate split tiles for each child. 
*/

typedef struct dynamic_terrain_tree_node {
	int depth;
	//double dist; //Just keep around while I adapt the code, remove later. //TODO: Completely remove this.
	void *tile;
	struct dynamic_terrain_tree_node *children[TERRAIN_TREE_NUM_CHILDREN];
} terrain_tree_node;

typedef struct terrain_tree_drawlist_node *terrain_tree_drawlist;
struct terrain_tree_drawlist_node {
	struct terrain_tree_drawlist_node *next;
	void *tile;
};

typedef int (*terrain_tree_depth_fn)(terrain_tree_node *, void *);
typedef bool (*terrain_tree_find_fn)(terrain_tree_node *, void *);
typedef void (*terrain_tree_split_fn)(void *parent, void **children[TERRAIN_TREE_NUM_CHILDREN], void *context);
typedef void (*terrain_tree_free_fn)(void *data);

terrain_tree_node * terrain_tree_new(void *tile, int depth);
void terrain_tree_gen(terrain_tree_node *tree, terrain_tree_depth_fn subdiv, terrain_tree_split_fn split, void *context);
void * terrain_tree_find(terrain_tree_node *tree, terrain_tree_find_fn find, void *context);
void terrain_tree_prune(terrain_tree_node *tree, terrain_tree_depth_fn subdiv, void *context, void (*free_data)(void *));
void terrain_tree_free(terrain_tree_node *tree, void (*free_data)(void *data));

void terrain_tree_drawlist_new(terrain_tree_node *tree, terrain_tree_depth_fn subdiv, void *context, terrain_tree_drawlist *list);
void terrain_tree_drawlist_free(terrain_tree_drawlist list);

#endif