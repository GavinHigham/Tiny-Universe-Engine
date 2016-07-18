#ifndef DYNAMIC_TERRAIN_H
#define DYNAMIC_TERRAIN_H
#include <stdlib.h>
#include <stdbool.h>
#include <glalgebra.h>
#include "buflist.h"

enum {
	NUM_CHILDREN = 4
};

typedef struct dynamic_terrain_node *PDTNODE;
typedef struct dynamic_terrain_node {
	amat4 frame;
	float dist; //Distance to camera
	PDTNODE children[NUM_CHILDREN];
	vec3 verts[3];
} *PDTNODE;

// struct dynamic_terrain_parameters {
	
// };

int dt_depth_per_distance(float distance);
int dt_node_distance_compare(const void *n1, const void *n2);
int dt_node_closeness_compare(const void *n1, const void *n2);
int dt_add_children(PDTNODE root, STACK *pool, HEAP *drawlist, vec3 cam_pos, int depth);

#endif