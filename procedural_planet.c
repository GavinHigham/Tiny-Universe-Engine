#include <GL/glew.h>
#include <stdlib.h>
#include <math.h>
#include "procedural_planet.h"
#include "triangular_terrain_tile.h"
//#include "dynamic_terrain.h"
#include "dynamic_terrain_tree.h"
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

// Static Functions //


extern float screen_width;
//I can remove some arguments to this and replace them with a single scale factor.
//This argument could be easily adjusted if I want some empirical pixel size per tri.
static int subdivisions_per_distance(float distance, float scale)
{
	return fmin(log2(scale/distance), MAX_SUBDIVISIONS);
}

static int dot_div_depth(vec3 tile_pos, vec3 planet_center, vec3 cam_pos)
{
	float d = fmax(0, pow(vec3_dot(vec3_normalize(tile_pos - planet_center), vec3_normalize(cam_pos - planet_center)), 16));
	return pow(MAX_SUBDIVISIONS, d);
}

static float split_tile_radius(int depth, float base_length)
{
	return base_length / pow(2, depth);
}

static int subdivision_depth(terrain_tree_node *tree, vec3 cam_pos, proc_planet *planet)
{
	float h = vec3_dist(cam_pos, planet->pos) - planet->radius; //If negative, we're below sea level.
	h = fmax(h, 0); //Don't handle "within the planet" case yet.
	float d = sqrt(2*planet->radius*h + h*h); //This is the distance to the horizon on a spherical planet.
	if (tree->dist > d + split_tile_radius(tree->depth, planet->edge_len)) //Node is beyond the horizon.
		return 0;

	float scale = (screen_width * planet->edge_len) / (2 * 40 * DEFAULT_NUM_TRI_TILE_ROWS);
	return subdivisions_per_distance(h, scale);
}

int terrain_tree_example_subdiv(terrain_tree_node *tree, void *context)
{
	struct planet_terrain_context *ctx = (struct planet_terrain_context *)context;
	tri_tile *tile = (tri_tile *)tree->tile;
	tree->dist = vec3_dist(tile->pos, ctx->cam_pos);

	//Cap the number of subdivisions per whole-tree traversal (per frame essentially)
	if (tree->depth == 0)
		ctx->subdivs_left = YIELD_AFTER_DIVS;
	if (ctx->subdivs_left <= 0)
	 	return 0;

	int depth = subdivision_depth(tree, ctx->cam_pos, ctx->planet);
	if (tree->depth < depth)
		ctx->subdivs_left--;
	return depth;
}

// Public Functions //

void proc_planet_drawlist(proc_planet *p, terrain_tree_drawlist *list, vec3 camera_position)
{
	struct planet_terrain_context context = {
		.subdivs_left = 0,
		.cam_pos = camera_position,
		.planet = p
	};

	for (int i = 0; i < NUM_ICOSPHERE_FACES; i++) {
		terrain_tree_gen(p->tiles[i], terrain_tree_example_subdiv, &context, (terrain_tree_split_fn)tri_tile_split);
		terrain_tree_drawlist_new(p->tiles[i], terrain_tree_example_subdiv, &context, list);
		terrain_tree_prune(p->tiles[i], terrain_tree_example_subdiv, &context, (terrain_tree_free_fn)free_tri_tile);
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
		vec3 verts[] = {
			ico_v[ico_i[3*i]]   * radius + pos,
			ico_v[ico_i[3*i+1]] * radius + pos,
			ico_v[ico_i[3*i+2]] * radius + pos
		};

		p->tiles[i] = terrain_tree_new(new_tri_tile(), 0);
		init_tri_tile(p->tiles[i]->tile, verts, DEFAULT_NUM_TRI_TILE_ROWS, up, pos, radius);
		gen_tri_tile_vertices_and_normals(p->tiles[i]->tile, height);
	}

	return p;
}

void free_proc_planet(proc_planet *p)
{
	for (int i = 0; i < NUM_ICOSPHERE_FACES; i++)
		terrain_tree_free(p->tiles[i], (terrain_tree_free_fn)free_tri_tile);
	free(p);
}