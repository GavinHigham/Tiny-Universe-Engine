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
#include "macros.h"

//Adapted from http://www.glprogramming.com/red/chapter02.html

static const float x = 0.525731112119133606;
static const float z = 0.850650808352039932;
extern space_sector eye_sector;
extern space_sector tri_sector;
 
static const vec3 ico_v[] = {    
	{-x, 0, z}, { x, 0,  z}, {-x,  0,-z}, { x,  0, -z},
	{ 0, z, x}, { 0, z, -x}, { 0, -z, x}, { 0, -z, -x},
	{ z, x, 0}, {-z, x,  0}, { z, -x, 0}, {-z, -x,  0}
};

static const GLuint ico_i[] = { 
	0,4,1,   0,9,4,   9,5,4,   4,5,8,   4,8,1,    
	8,10,1,  8,3,10,  5,3,8,   5,2,3,   2,7,3,    
	7,10,3,  7,6,10,  7,11,6,  11,0,6,  0,1,6, 
	6,1,10,  9,0,11,  9,11,2,  9,2,5,   7,2,11
};

const vec3 proc_planet_up = (vec3){z/3, (z+z+x)/3, 0}; //Centroid of ico_i[3]
const vec3 proc_planet_not_up = (vec3){-(z+z+x)/3, z/3, 0};

// Static Functions //

tri_tile * tree_tile(terrain_tree_node *tree)
{
	return (tri_tile *)tree->tile;
}

extern float screen_width;
//I can remove some arguments to this and replace them with a single scale factor.
//This argument could be easily adjusted if I want some empirical pixel size per tri.
static int subdivisions_per_distance(float distance, float scale)
{
	return fmin(log2(scale/distance), MAX_SUBDIVISIONS);
}

// static int dot_div_depth(vec3 tile_pos, vec3 planet_center, vec3 cam_pos)
// {
// 	float d = fmax(0, pow(vec3_dot(vec3_normalize(tile_pos - planet_center), vec3_normalize(cam_pos - planet_center)), 16));
// 	return pow(MAX_SUBDIVISIONS, d);
// }

static float split_tile_radius(int depth, float base_length)
{
	return base_length / pow(2, depth);
}

//Distance to the horizon with a planet radius R and elevation above sea level h, from Wikipedia.
float distance_to_horizon(float R, float h)
{
	return sqrt(h * (2 * R + h));
}

static int subdivision_depth(terrain_tree_node *tree, dvec3 cam_pos, proc_planet *planet)
{
	//If negative, we're below sea level.
	float h = dvec3_mag(cam_pos) - planet->radius;
	//Don't handle "within the planet" case yet.
	float d = distance_to_horizon(planet->radius, fmax(h, 0));
	//Don't subdivide if the tile center is "tile radius" distance beyond the horizon.
	float tile_radius = split_tile_radius(tree->depth, planet->edge_len);
	//printf("Tile dist, horizon dist, depth: %10f, %10f, %i\n", tree->dist - tile_radius, d, tree->depth);
	if (tree->dist - tile_radius > d) {
		tree_tile(tree)->override_col = (vec3){0.0, 1.0, 0.0};
		return 0;
	} else {
		tree_tile(tree)->override_col = (vec3){1.0, 1.0, 1.0};
	}

	float scale = (screen_width * planet->edge_len) / (2 * 40 * DEFAULT_NUM_TRI_TILE_ROWS);

	return subdivisions_per_distance(fmax(h, 0), scale);
}

int terrain_tree_example_subdiv(terrain_tree_node *tree, void *context)
{
    //return 0;
	struct planet_terrain_context *ctx = (struct planet_terrain_context *)context;

    //Cap the number of subdivisions per whole-tree traversal (per frame essentially)
	//TODO: Check this expression, make sure it can't cause a whole-tree traversal by mistake.
	if (tree->depth == 0)
		ctx->splits_left = YIELD_AFTER_DIVS;
	if (ctx->splits_left <= 0)
	 	return 0;

	ctx->visited++;
	if (ctx->visited > 200)
		return 0;

	tri_tile *tile = tree_tile(tree);
	//The camera position and sector were provided relative to the planet, like the tile centroid and sector.
	//Convert the camera position to be relative to the tile, and store the distance from camera to tile in the tree node.
	//printf("Cam: "); vec3_print(space_sector_position_relative_to_sector(ctx->cam_pos, ctx->cam_sec, tile->sector)); printf(", Centroid: "); vec3_print(tile->centroid); printf("\n");
	dvec3 dcent = {tile->centroid.x, tile->centroid.y, tile->centroid.z};
	dvec3 dcam = {ctx->cam_pos.x, ctx->cam_pos.y, ctx->cam_pos.z};
	tree->dist = dvec3_dist(dcent, space_sector_dposition_relative_to_sector(dcam, ctx->cam_sec, tile->sector));

	//This calculation might be hitting the limits of floating-point precision; the center of the planet is simply too far from the camera.
	dcam = space_sector_dposition_relative_to_sector(dcam, ctx->cam_sec, (space_sector){0, 0, 0});
	return subdivision_depth(tree, dcam, ctx->planet);
}

vec3 primary_color_by_depth[] = {
	{1.0, 0.0, 0.0}, //Red
	{0.5, 0.5, 0.0}, //Yellow
	{0.0, 1.0, 0.0}, //Green
	{0.0, 0.5, 0.5}, //Cyan
	{0.0, 0.0, 1.0}, //Blue
	{0.5, 0.0, 0.5}, //Purple
};

tri_tile * proc_planet_vertices_and_normals(tri_tile *t, height_map_func height, vec3 planet_pos, float noise_radius, float amplitude)
{
	vec3 brownish = {0.30, .27, 0.21};
	vec3 whiteish = {0.96, .94, 0.96};
	vec3 primary_color = {1, 1, 1}; //White //primary_color_by_depth[t->depth % LENGTH(primary_color_by_depth)];
	float epsilon = noise_radius / 100000; //TODO: Check this value or make it empirical somehow.
	for (int i = 0; i < t->num_vertices; i++) {
		//Points towards vertex from planet origin
		vec3 pos = t->positions[i] - planet_pos;
		float m = vec3_mag(pos);
		//Point at the surface of our simulated smaller planet.
		vec3 noise_surface = pos * (noise_radius/m);

		//Basis vectors
		//TODO: Use a different "up" vector on the poles.
		vec3 x = vec3_normalize(vec3_cross(proc_planet_up, pos));
		vec3 z = vec3_normalize(vec3_cross(pos, x));
		vec3 y = vec3_normalize(pos);

		//Calculate position and normal on a sphere of noise_radius radius.
		tri_tile_vertex_position_and_normal(height, x, y, z, epsilon, &noise_surface, &t->normals[i]);
		//Scale back up to planet size.
		t->positions[i] = noise_surface * (m/noise_radius) + planet_pos;
		//TODO: Create much more interesting colors.
		t->colors[i] = primary_color * vec3_lerp(brownish, whiteish, fmax((vec3_dist(noise_surface, (vec3){0,0,0}) - noise_radius) / amplitude, 0.0));
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
	vec3 planet_pos = space_sector_position_relative_to_sector((vec3){0, 0, 0}, (space_sector){0, 0, 0}, t->sector);
	//Curve the tile around planet by normalizing each vertex's distance to the planet and scaling by planet radius.
	reproject_vertices_to_spherical(t->positions, t->num_vertices, planet_pos, p->radius);
	//Apply perturbations to the surface and calculate normals.
	//Since noise doesn't compute well on huge planets, noise is calculated on a simulated smaller planet and scaled up.
	proc_planet_vertices_and_normals(t, p->height, planet_pos, p->noise_radius, p->amplitude);
}

void tri_tile_split(tri_tile *t, tri_tile **out[DEFAULT_NUM_TRI_TILE_DIVS], void *context)
{
	tri_tile *tmp[DEFAULT_NUM_TRI_TILE_DIVS];
	for (int i = 0; i < DEFAULT_NUM_TRI_TILE_DIVS; i++) {
		*out[i] = new_tri_tile();
		(*out[i])->depth = t->depth + 1;
		tmp[i] = *out[i];
	}
	((struct planet_terrain_context *)context)->splits_left--;

	vec3 new_tile_vertices[] = {
		vec3_lerp(t->tile_vertices[0], t->tile_vertices[1], 0.5),
		vec3_lerp(t->tile_vertices[0], t->tile_vertices[2], 0.5),
		vec3_lerp(t->tile_vertices[1], t->tile_vertices[2], 0.5)
	};

	proc_planet *planet = (proc_planet *)t->finishing_touches_context;
	vec3 planet_pos = space_sector_position_relative_to_sector((vec3){0, 0, 0}, (space_sector){0, 0, 0}, t->sector);
	reproject_vertices_to_spherical(new_tile_vertices, 3, planet_pos, planet->radius);

	init_tri_tile(*out[0], (vec3[3]){t->tile_vertices[0], new_tile_vertices[0], new_tile_vertices[1]}, t->sector, DEFAULT_NUM_TRI_TILE_ROWS, t->finishing_touches, t->finishing_touches_context);
	init_tri_tile(*out[1], (vec3[3]){new_tile_vertices[0], t->tile_vertices[1], new_tile_vertices[2]}, t->sector, DEFAULT_NUM_TRI_TILE_ROWS, t->finishing_touches, t->finishing_touches_context);
	init_tri_tile(*out[2], (vec3[3]){new_tile_vertices[0], new_tile_vertices[2], new_tile_vertices[1]}, t->sector, DEFAULT_NUM_TRI_TILE_ROWS, t->finishing_touches, t->finishing_touches_context);
	init_tri_tile(*out[3], (vec3[3]){new_tile_vertices[1], new_tile_vertices[2], t->tile_vertices[2]}, t->sector, DEFAULT_NUM_TRI_TILE_ROWS, t->finishing_touches, t->finishing_touches_context);
	printf("Dividing %p, depth %i into %d new tiles.\n", t, t->depth, DEFAULT_NUM_TRI_TILE_DIVS);
}

// Public Functions //

proc_planet * proc_planet_new(float radius, height_map_func height)
{
	proc_planet *p = malloc(sizeof(proc_planet));
	p->radius = radius;
	p->noise_radius = 6000; //TODO: Determine the largest reasonable noise radius, map input radius to a good range.
	p->amplitude = TERRAIN_AMPLITUDE; //TODO: Choose a good number, pass this through the chain of calls.
	p->edge_len = radius / sin(2.0*M_PI/5.0);
	printf("Edge len: %f\n", p->edge_len);
	p->height = height;
	for (int i = 0; i < NUM_ICOSPHERE_FACES; i++) {
		vec3 verts[] = {
			ico_v[ico_i[3*i]]   * radius,
			ico_v[ico_i[3*i+1]] * radius,
			ico_v[ico_i[3*i+2]] * radius
		};

		p->tiles[i] = terrain_tree_new(new_tri_tile(), 0);
		//Initialize tile with verts expressed relative to p->sector.
		init_tri_tile((tri_tile *)p->tiles[i]->tile, verts, (space_sector){0, 0, 0}, DEFAULT_NUM_TRI_TILE_ROWS, &proc_planet_finishing_touches, p);
	}

	return p;
}

void proc_planet_free(proc_planet *p)
{
	for (int i = 0; i < NUM_ICOSPHERE_FACES; i++)
		terrain_tree_free(p->tiles[i], (terrain_tree_free_fn)free_tri_tile);
	free(p);
}

void proc_planet_drawlist(proc_planet *p, terrain_tree_drawlist *list, vec3 cam_pos_offset, space_sector cam_sector_offset)
{
	struct planet_terrain_context context = {
		.splits_left = 5, //Total number of splits for this call of drawlist.
		.cam_pos = cam_pos_offset,
		.cam_sec = cam_sector_offset,
		.planet = p,
		.visited = 0
	};

	vec3 cam_pos = space_sector_position_relative_to_sector(cam_pos_offset, cam_sector_offset, (space_sector){0, 0, 0});

	//TODO: Check the distance here and draw an imposter instead of the whole planet if it's far enough.

	for (int i = 0; i < NUM_ICOSPHERE_FACES; i++) {
		tri_tile *t = tree_tile(p->tiles[i]);
		//Ignore tiles that are facing away from the camera. Can remove when my horizon check is working right.
		if (vec3_dot(t->normal, vec3_normalize(cam_pos)) > -0.1) {
			terrain_tree_gen(p->tiles[i], terrain_tree_example_subdiv, (terrain_tree_split_fn)tri_tile_split, &context);
			terrain_tree_drawlist_new(p->tiles[i], terrain_tree_example_subdiv, &context, list);
			//terrain_tree_prune(p->tiles[i], terrain_tree_example_subdiv, &context, (terrain_tree_free_fn)free_tri_tile);
		}
	}

	printf("%d\n", context.visited);
}
