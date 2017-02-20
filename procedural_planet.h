#ifndef PROCEDURAL_PLANET_H
#define PROCEDURAL_PLANET_H
#include <glla.h>
#include "dynamic_terrain_types.h"
#include "dynamic_terrain_tree.h"
//#include "terrain_types.h"

typedef struct procedural_planet {
	vec3 pos;
	float radius;
	float edge_len;
	terrain_tree_node *tiles[NUM_ICOSPHERE_FACES];
	height_map_func height;
} proc_planet;

struct planet_terrain_context {
	int subdivs_left;
	vec3 cam_pos;
	proc_planet *planet;
};

proc_planet * new_proc_planet(vec3 pos, float radius, height_map_func height);
void free_proc_planet(proc_planet *p);
void proc_planet_drawlist(proc_planet *p, terrain_tree_drawlist *list, vec3 camera_position);

#endif
