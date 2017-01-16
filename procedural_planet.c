#include <GL/glew.h>
#include <stdlib.h>
#include <math.h>
#include "procedural_planet.h"
#include "triangular_terrain_tile.h"
#include "dynamic_terrain.h"
#include "terrain_constants.h"

//Adapted from http://www.glprogramming.com/red/chapter02.html

static const float x = 0.525731112119133606;
static const float z = 0.850650808352039932;
 
static vec3 ico_v[] = {    
	{-x, 0.0, z}, {x, 0.0, z},   {-x, 0.0, -z}, {x, 0.0, -z} , 
	{0.0, z, x},  {0.0, z, -x}, {0.0, -z, x},  {0.0, -z, -x}, 
	{z, x, 0.0},  {-z, x, 0.0},  {z, -x, 0.0},  {-z, -x, 0.0}
};

static const GLuint ico_i[] = { 
	0,4,1,   0,9,4,   9,5,4,   4,5,8,   4,8,1,    
	8,10,1,  8,3,10,  5,3,8,   5,2,3,   2,7,3,    
	7,10,3,  7,6,10,  7,11,6,  11,0,6,  0,1,6, 
	6,1,10,  9,0,11,  9,11,2,  9,2,5,   7,2,11
};

void proc_planet_drawlist(proc_planet *p, DRAWLIST *terrain_list, vec3 camera_position)
{
	for (int i = 0; i < NUM_ICOSPHERE_FACES; i++) {
		subdivide_tree(p->tiles[i], camera_position, p);
		create_drawlist(p->tiles[i], terrain_list, camera_position, p);
		prune_tree(p->tiles[i], camera_position, p);
	}
}

proc_planet * new_proc_planet(vec3 pos, float radius, height_map_func height)
{
	vec3 up = {0, 1, 0}; //TODO: Choose a better "up"
	proc_planet *p = malloc(sizeof(proc_planet));
	p->pos = pos;
	p->radius = radius;
	p->edge_len = radius / sin(2.0*M_PI/5.0);
	p->height = height;
	for (int i = 0; i < NUM_ICOSPHERE_FACES; i++) {
		tri_tile t = {.is_init = false};
		vec3 verts[] = {
			vec3_add(vec3_scale(ico_v[ico_i[3*i]],   radius), pos),
			vec3_add(vec3_scale(ico_v[ico_i[3*i+1]], radius), pos),
			vec3_add(vec3_scale(ico_v[ico_i[3*i+2]], radius), pos)};

		init_tri_tile(&t, verts, DEFAULT_NUM_TRI_TILE_ROWS, up, pos, radius);
		gen_tri_tile_vertices_and_normals(&t, height);
		p->tiles[i] = new_tree(t, 0);
	}

	return p;
}

void free_proc_planet(proc_planet *p)
{
	for (int i = 0; i < NUM_ICOSPHERE_FACES; i++)
		free_tree(p->tiles[i]);
	free(p);
}