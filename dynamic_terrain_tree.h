//INLINE HEADER FILE

#ifndef DYNAMIC_TERRAIN_TREE_H
#define DYNAMIC_TERRAIN_TREE_H

#include <stdlib.h>
#include <stdbool.h>
#include <glla.h>

/*
Subdivide, drawlist, and prune take a depth-definining function, and a void * context.
The depth function determines from the tree node and data retreived from the context
what depth the node should be subdivided to. Subdivide calls this function at every node
visited, checking if the node has been divided enough.

Subdivide takes an additional "split" 
*/

typedef struct dynamic_terrain_tree_node {
	int depth;
	void *tile;
} terrain_tree_node;

struct planet_terrain_context {
	int subdivs_left;
	vec3 cam_pos;
	proc_planet *planet;
};

DRAWLIST drawlist_prepend(DRAWLIST list, tri_tile *t);
void drawlist_free(DRAWLIST list);

int dt_depth_per_distance(float distance);
int dt_node_distance_compare(const void *n1, const void *n2);
int dt_node_closeness_compare(const void *n1, const void *n2);

PDTNODE new_tree(tri_tile t, int depth);
void subdivide_tree(terrain_tree_node *tree, bool (subdiv)(terrain_tree_node *, void *), void *subdiv_ctx);
void create_drawlist(PDTNODE tree, DRAWLIST *drawlist, vec3 cam_pos, proc_planet *planet);
void prune_tree(terrain_tree_node *tree, bool (subdiv)(terrain_tree_node *, void *), void *subdiv_ctx);
void free_tree(PDTNODE tree);

#endif