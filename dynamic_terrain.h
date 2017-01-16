#ifndef DYNAMIC_TERRAIN_H
#define DYNAMIC_TERRAIN_H

#include <stdlib.h>
#include <stdbool.h>
#include <glla.h>
#include "terrain_types.h"

DRAWLIST drawlist_prepend(DRAWLIST list, tri_tile *t);
void drawlist_free(DRAWLIST list);

int dt_depth_per_distance(float distance);
int dt_node_distance_compare(const void *n1, const void *n2);
int dt_node_closeness_compare(const void *n1, const void *n2);

PDTNODE new_tree(tri_tile t, int depth);
void subdivide_tree(PDTNODE tree, vec3 cam_pos, proc_planet *planet);
void create_drawlist(PDTNODE tree, DRAWLIST *drawlist, vec3 cam_pos, proc_planet *planet);
void prune_tree(PDTNODE tree, vec3 cam_pos, proc_planet *planet);
void free_tree(PDTNODE tree);

#endif

/*
Algorithm details:

Traverse the tree, subdividing nodes that need more resolution.

Traverse the tree again, gathering a list of nodes that are to be drawn (in frustrum, and correct subdiv level).
Buffer nodes that are not on the GPU (and should be).

Traverse the tree a third time, gathering a list of nodes that could be deleted
	these nodes should be ordered by distance from the camera, with all child nodes coming before their parents in the ordering
	Can I take advantage of the fact that the distance of child nodes will be strongly correlated with the distance of their parents?
	Delete GPU buffers until GPU memory constraints are satisfied. Delete vertex data until system memory constraints are satisfied.

Later optimizations:
	Pool of GPU buffers and nodes, "free" to pool, and "new" from it
	Keep a list of potential nodes to be deleted, delete or recycle on-demand as needed
*/