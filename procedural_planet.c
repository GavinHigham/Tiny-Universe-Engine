#include <GL/glew.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include "procedural_planet.h"
#include "triangular_terrain_tile.h"
//#include "dynamic_terrain.h"
#include "dynamic_terrain_tree.h"
#include "terrain_constants.h"
#include "math/space_sector.h"

//Adapted from http://www.glprogramming.com/red/chapter02.html

static const float x = 0.525731112119133606;
static const float z = 0.850650808352039932;
const vec3 proc_planet_up = {0, 1, 0};
const vec3 proc_planet_right = {1, 0, 0};
extern space_sector eye_sector;
extern space_sector tri_sector;
 
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
	return 0;
	struct planet_terrain_context *ctx = (struct planet_terrain_context *)context;
	tri_tile *tile = (tri_tile *)tree->tile;
	tree->dist = vec3_dist(space_sector_position_relative_to_sector(tile->pos, tile->sector, ctx->cam_sec), ctx->cam_pos);

	//Cap the number of subdivisions per whole-tree traversal (per frame essentially)
	if (tree->depth == 0)
		ctx->subdivs_left = YIELD_AFTER_DIVS;
	if (ctx->subdivs_left <= 0)
	 	return 0;

	int depth = subdivision_depth(tree, space_sector_position_relative_to_sector(ctx->cam_pos, ctx->cam_sec, ctx->planet->sector), ctx->planet);
	if (tree->depth < depth)
		ctx->subdivs_left--;
	return depth;
}

vec3 planet_pos_relative_to_tile(tri_tile *t)
{
	proc_planet *planet = (proc_planet *)t->finishing_touches_context;
	return space_sector_position_relative_to_sector(planet->pos, planet->sector, t->sector);
}

tri_tile * proc_planet_vertices_and_normals(tri_tile *t, height_map_func height, vec3 planet_pos, float noise_radius, float amplitude)
{
	vec3 brownish = {0.30, .27, 0.21};
	vec3 whiteish = {0.96, .94, 0.96};
	float epsilon = noise_radius / 100000; //TODO: Check this value or make it empirical somehow.
	for (int i = 0; i < t->num_vertices; i++) {
		//Points towards vertex
		vec3 pos = t->positions[i] - planet_pos;
		//Point at the surface of our simulated smaller planet.
		vec3 noise_surface = vec3_scale(vec3_normalize(pos), noise_radius);
		//This will be distorted by the heightmap function.
		vec3 displaced_surface = noise_surface;

		//Basis vectors
		//TODO: Use a different "up" vector on the poles.
		vec3 x = vec3_normalize(vec3_cross(proc_planet_up, pos));
		vec3 z = vec3_normalize(vec3_cross(pos, x));
		vec3 y = vec3_normalize(pos);

		//Calculate position and normal on a sphere of noise_radius radius.
		tri_tile_vertex_position_and_normal(height, x, y, z, epsilon, &displaced_surface, &t->normals[i]);
		//Scale back up to planet size.
		//UNCOMMENT
		t->positions[i] += displaced_surface - noise_surface;
		//TODO: Create much more interesting colors.
		t->colors[i] = vec3_lerp(brownish, whiteish, fmax((vec3_dist(noise_surface, (vec3){0,0,0}) - noise_radius) / amplitude, 0.0));
	}
	return t;
}

void reproject_vertices_to_spherical(vec3 vertices[], int num_vertices, vec3 pos, float radius)
{
	for (int i = 0; i < num_vertices; i++) {
		vec3 d = vertices[i] - pos;
		vertices[i] = d * radius/vec3_mag(d) + pos;
	}
}

void proc_planet_finishing_touches(tri_tile *t, void *finishing_touches_context)
{
	//Retrieve tile's planet from finishing_touches_context.
	proc_planet *p = (proc_planet *)finishing_touches_context;
	vec3 planet_pos = planet_pos_relative_to_tile(t);
	//Curve the tile around planet by normalizing each vertex's distance to the planet and scaling by planet radius.
	reproject_vertices_to_spherical(t->positions, t->num_vertices, planet_pos, p->radius);
	//Apply perturbations to the surface and calculate normals.
	//Since noise doesn't compute well on huge planets, noise is calculated on a simulated smaller planet and scaled up.
	proc_planet_vertices_and_normals(t, p->height, planet_pos, p->noise_radius, p->amplitude);
}

void subdiv_tri_tile(tri_tile *in, tri_tile *out[DEFAULT_NUM_TRI_TILE_DIVS])
{
	vec3 new_tile_vertices[] = {
		vec3_lerp(in->tile_vertices[0], in->tile_vertices[1], 0.5),
		vec3_lerp(in->tile_vertices[0], in->tile_vertices[2], 0.5),
		vec3_lerp(in->tile_vertices[1], in->tile_vertices[2], 0.5)
	};

	proc_planet *planet = (proc_planet *)in->finishing_touches_context;
	vec3 planet_pos = planet_pos_relative_to_tile(in);
	reproject_vertices_to_spherical(new_tile_vertices, 3, planet_pos, planet->radius);

	init_tri_tile(out[0], (vec3[3]){in->tile_vertices[0], new_tile_vertices[0], new_tile_vertices[1]}, in->sector, DEFAULT_NUM_TRI_TILE_ROWS, in->finishing_touches, in->finishing_touches_context);
	init_tri_tile(out[1], (vec3[3]){new_tile_vertices[0], in->tile_vertices[1], new_tile_vertices[2]}, in->sector, DEFAULT_NUM_TRI_TILE_ROWS, in->finishing_touches, in->finishing_touches_context);
	init_tri_tile(out[2], (vec3[3]){new_tile_vertices[0], new_tile_vertices[2], new_tile_vertices[1]}, in->sector, DEFAULT_NUM_TRI_TILE_ROWS, in->finishing_touches, in->finishing_touches_context);
	init_tri_tile(out[3], (vec3[3]){new_tile_vertices[1], new_tile_vertices[2], in->tile_vertices[2]}, in->sector, DEFAULT_NUM_TRI_TILE_ROWS, in->finishing_touches, in->finishing_touches_context);

	printf("Created 4 new terrains from terrain %p\n", in);
}

void tri_tile_split(tri_tile *in, tri_tile **out[DEFAULT_NUM_TRI_TILE_DIVS])
{
	tri_tile *tmp[DEFAULT_NUM_TRI_TILE_DIVS];
	for (int i = 0; i < DEFAULT_NUM_TRI_TILE_DIVS; i++) {
		*out[i] = new_tri_tile();
		tmp[i] = *out[i];
	}
	subdiv_tri_tile(in, tmp);
}

// Public Functions //

proc_planet * proc_planet_new(vec3 pos, space_sector sector, float radius, height_map_func height)
{
	proc_planet *p = malloc(sizeof(proc_planet));
	space_sector_canonicalize(&pos, &sector);
	p->pos = pos;
	p->sector = sector;
	p->radius = radius;
	p->noise_radius = 6000; //TODO: Determine the largest reasonable noise radius, map input radius to a good range.
	p->amplitude = TERRAIN_AMPLITUDE; //TODO: Choose a good number, pass this through the chain of calls.
	p->edge_len = radius / sin(2.0*M_PI/5.0);
	p->height = height;
	for (int i = 0; i < NUM_ICOSPHERE_FACES; i++) {
		vec3 verts[] = {
			ico_v[ico_i[3*i]]   * radius + pos,
			ico_v[ico_i[3*i+1]] * radius + pos,
			ico_v[ico_i[3*i+2]] * radius + pos
		};

		p->tiles[i] = terrain_tree_new(new_tri_tile(), 0);
		//Initialize tile with verts expressed relative to p->sector.
		init_tri_tile(p->tiles[i]->tile, verts, p->sector, DEFAULT_NUM_TRI_TILE_ROWS, &proc_planet_finishing_touches, p);
	}

	return p;
}

void proc_planet_free(proc_planet *p)
{
	for (int i = 0; i < NUM_ICOSPHERE_FACES; i++)
		terrain_tree_free(p->tiles[i], (terrain_tree_free_fn)free_tri_tile);
	free(p);
}

void proc_planet_drawlist(proc_planet *p, terrain_tree_drawlist *list, vec3 camera_position, space_sector camera_sector)
{
	struct planet_terrain_context context = {
		.subdivs_left = 0,
		//TODO: Revisit the sectors used here. The planet, and each tile, have their own sectors now.
		.cam_pos = camera_position,
		.cam_sec = camera_sector,
		.planet = p
	};

	//TODO: Check the distance here and draw an imposter instead of the whole planet if it's far enough.

	for (int i = 0; i < NUM_ICOSPHERE_FACES; i++) {
		terrain_tree_gen(p->tiles[i], terrain_tree_example_subdiv, &context, (terrain_tree_split_fn)tri_tile_split);
		terrain_tree_drawlist_new(p->tiles[i], terrain_tree_example_subdiv, &context, list);
		terrain_tree_prune(p->tiles[i], terrain_tree_example_subdiv, &context, (terrain_tree_free_fn)free_tri_tile);
	}
}