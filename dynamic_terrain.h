#ifndef DYNAMIC_TERRAIN_H
#define DYNAMIC_TERRAIN_H
#include <glalgebra.h>

typedef struct dynamic_terrain_node *PDTNODE;
typedef struct dynamic_terrain_node {
	vec3 pos; //Global position
	float dist; //Distance to camera
	PDTNODE children[4];
} *PDTNODE;

dt_add_children(PDTNODE root, vec3 cam_pos) {
	root->dist = vec3_dist(root->pos, cam_pos);
	//if the camera is closer than some threshold, generate child nodes, then call self on children
}

#endif