#ifndef PROCEDURAL_PLANET_H
#define PROCEDURAL_PLANET_H
#include "glla.h"
#include "dynamic_terrain_tree.h"
#include "triangular_terrain_tile.h"
#include "terrain_constants.h"
#include "math/space_sector.h"

// //Used to generate normals for most of the planet.
// extern const vec3 proc_planet_up;
// //Used to generate normals for the poles and any descendant tiles.
// extern const vec3 proc_planet_not_up;

typedef struct procedural_planet {
	vec3 up;
	vec3 right;
	float radius;
	float noise_radius;
	float amplitude;
	float edge_len;
	//TODO: Consolidate these.
	vec3 colors[3];
	vec3 color_family;
	terrain_tree_node *tiles[NUM_ICOSPHERE_FACES];
	height_map_func height;
	struct osn_context *osnctx;
} proc_planet;

struct planet_terrain_context {
	int splits_left;
	vec3 cam_pos;
	space_sector cam_sec;
	proc_planet *planet;
	int visited;
};

proc_planet * proc_planet_new(float radius, height_map_func height, vec3 color_family);
void proc_planet_free(proc_planet *p);
void proc_planet_drawlist(proc_planet *p, terrain_tree_drawlist *list, vec3 camera_position, space_sector camera_sector);
float proc_planet_height(struct osn_context *ctx, vec3 pos, vec3 *variety);

#endif
