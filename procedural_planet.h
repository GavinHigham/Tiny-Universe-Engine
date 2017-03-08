#ifndef PROCEDURAL_PLANET_H
#define PROCEDURAL_PLANET_H
#include "glla.h"
#include "dynamic_terrain_types.h"
#include "dynamic_terrain_tree.h"
#include "math/space_sector.h"

// //Used to generate normals for most of the planet.
// extern const vec3 proc_planet_up;
// //Used to generate normals for the poles and any descendant tiles.
// extern const vec3 proc_planet_not_up;

typedef struct procedural_planet {
	vec3 pos;
	vec3 up;
	vec3 right;
	space_sector sector;
	float radius;
	float noise_radius;
	float amplitude;
	float edge_len;
	terrain_tree_node *tiles[NUM_ICOSPHERE_FACES];
	height_map_func height;
} proc_planet;

struct planet_terrain_context {
	int subdivs_left;
	vec3 cam_pos;
	space_sector cam_sec;
	proc_planet *planet;
};

proc_planet * proc_planet_new(vec3 pos, space_sector sector, float radius, height_map_func height);
void proc_planet_free(proc_planet *p);
void proc_planet_drawlist(proc_planet *p, terrain_tree_drawlist *list, vec3 camera_position, space_sector camera_sector);

#endif
