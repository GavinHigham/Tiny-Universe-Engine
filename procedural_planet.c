#include <GL/glew.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include "procedural_planet.h"
#include "math/geometry.h"
#include "macros.h"
#include "input_event.h" //For controller hotkeys
#include "open-simplex-noise-in-c/open-simplex-noise.h"

//Suppress prints
#define printf(...) while(0) {}

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

vec3 primary_color_by_depth[] = {
	{1.0, 0.0, 0.0}, //Red
	{0.5, 0.5, 0.0}, //Yellow
	{0.0, 1.0, 0.0}, //Green
	{0.0, 0.5, 0.5}, //Cyan
	{0.0, 0.0, 1.0}, //Blue
	{0.5, 0.0, 0.5}, //Purple
};

const vec3 proc_planet_up = (vec3){z/3, (z+z+x)/3, 0}; //Centroid of ico_i[3]
const vec3 proc_planet_not_up = (vec3){-(z+z+x)/3, z/3, 0};
extern float screen_width;
extern struct osn_context *osnctx;
static int64_t planet_seed = 1;

// Static Functions //

tri_tile * tree_tile(terrain_tree_node *tree)
{
	return (tri_tile *)tree->tile;
}

static int splits_per_distance(float distance, float scale)
{
	return fmax(fmin(log2(scale/distance), MAX_SUBDIVISIONS), 0);
}

static float split_tile_radius(int depth, float base_length)
{
	return base_length / pow(2, depth);
}

//Distance to the horizon with a planet radius R and elevation above sea level h, from Wikipedia.
float distance_to_horizon(float R, float h)
{
	return sqrt(2*R*h + h*h);
}

int proc_planet_split_depth(terrain_tree_node *tree, void *context)
{
	struct planet_terrain_context *ctx = (struct planet_terrain_context *)context;
	tri_tile *tile = tree_tile(tree);
	vec3 debug_col = (vec3){1.0, 1.0, 1.0};
	int depth = 0;
	ctx->visited++;

	//Cap the number of subdivisions per whole-tree traversal (per frame essentially)
	if (tree->depth == 0)
		ctx->splits_left = YIELD_AFTER_DIVS;
	if (ctx->splits_left <= 0)
	 	goto exit; //Return 0

	//Convert camera and tile position to planet-coordinates.
	//These calculations might hit the limits of floating-point precision if the planet is really large.
	vec3 tile_pos = space_sector_position_relative_to_sector(tile->centroid, tile->sector, (space_sector){0, 0, 0});
	vec3 cam_pos = space_sector_position_relative_to_sector(ctx->cam_pos, ctx->cam_sec, (space_sector){0, 0, 0});;
	vec3 surface_pos = cam_pos * ctx->planet->radius/vec3_mag(cam_pos);

	float altitude = vec3_mag(cam_pos) - ctx->planet->radius;
	float tile_dist = vec3_dist(surface_pos, tile_pos) - split_tile_radius(tree->depth, ctx->planet->edge_len);
	float horizon_dist = distance_to_horizon(ctx->planet->radius, fmax(altitude, 0));

	if (tile_dist > horizon_dist) {
		if (nes30_buttons[INPUT_BUTTON_SELECT])
			debug_col *= (vec3){0.2, 1.0, 0.2}; //Color the tile green for debug.
		goto exit; //Return 0
	}

	float subdiv_dist = INFINITY;
	if (altitude > tile_dist) {
		subdiv_dist = altitude;
		//debug_col *= (vec3){0.2, 0.2, 1.0}; //Color the tile blue for debug.
	} else {
		subdiv_dist = tile_dist;
		//debug_col *= (vec3){1.0, 0.2, 0.2}; //Color the tile red for debug.
	}

	float scale_factor = (screen_width * ctx->planet->edge_len) / (2 * PIXELS_PER_TRI * DEFAULT_NUM_TRI_TILE_ROWS);
	depth = splits_per_distance(subdiv_dist, scale_factor);

	if (nes30_buttons[INPUT_BUTTON_START])
		debug_col *= primary_color_by_depth[depth % LENGTH(primary_color_by_depth)];
exit:
	tile->override_col = debug_col;
	return depth;
}

//Using height, take position and distort it along the basis vectors, and compute its normal.
//height: A heightmap function which will affect the final position of the vertex along the basis_y vector.
//basis x, basis_y, basis_z: Basis vectors for the vertex.
//position: In/Out, the starting and ending position of the vertex.
//normal: Output for the normal of the vertex.
//Returns the "height", or displacement along basis_y.
float position_normal_color(height_map_func height, vec3 basis_x, vec3 basis_y, vec3 basis_z, float epsilon, vec3 *position, vec3 *normal, vec3 *color)
{
	//TODO: Move these into the planet struct.
	vec3 orangy = (vec3){203, 123, 78} / 255;
	vec3 greyish = (vec3){77, 110, 159} / 255;
	vec3 blueish = (vec3){96, 106, 87} / 255;

	//Create two points, scootched out along the basis vectors.
	vec3 pos1 = basis_x * epsilon + *position;
	vec3 pos2 = basis_z * epsilon + *position;

	vec3 color1, color2, c;
	//Find procedural heights, and add them.
	pos1      = basis_y * height(pos1, &color1) + pos1;
	pos2      = basis_y * height(pos2, &color2) + pos2;
	*position = basis_y * height(*position, &c) + *position;
	//Compute the normal.
	*normal = vec3_normalize(vec3_cross(pos1 - *position, pos2 - *position));
	c = (c + 1) / 2;
	*color = c.x * blueish + c.y * orangy * 3 + c.z * greyish;
	return position->y;
}

tri_tile * proc_planet_vertices_and_normals(vec3 primary_color, tri_tile *t, height_map_func height, vec3 planet_pos, float noise_radius, float amplitude)
{
	vec3 brownish = {0.30, .27, 0.21};
	//vec3 whiteish = {0.96, .94, 0.96};
	vec3 orangeish = (vec3){255, 181, 112} / 255;
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
		position_normal_color(height, x, y, z, epsilon, &noise_surface, &t->normals[i], &t->colors[i]);
		//Scale back up to planet size.
		t->positions[i] = noise_surface * (m/noise_radius) + planet_pos;
		//TODO: Create much more interesting colors.
		t->colors[i] *= primary_color * vec3_lerp(brownish, orangeish, fmax((vec3_dist(noise_surface, (vec3){0,0,0}) - noise_radius) / amplitude, 0.0));
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
	proc_planet_vertices_and_normals(p->color_family, t, p->height, planet_pos, p->noise_radius, p->amplitude);
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

float fbm(vec3 p)
{
	return open_simplex_noise3(osnctx, p.x, p.y, p.z);
}

float distorted_height(vec3 pos, vec3 *variety)
{
	vec3 a = {1.4, 2.08, 1.3};
	vec3 b = {13.8, 7.9, 2.1};

	vec3 h1 = {
		fbm(pos),
		fbm(pos + a),
		fbm(pos + b)
	};
	vec3 h2 = {
		fbm(pos + 2.4*h1),
		fbm(pos + 2*h1 + a),
		fbm(pos + 1*h1 + b)
	};

	*variety = h1 + h2 / 2;
	return fbm(pos + 0.65*h2);
}

float proc_planet_height(vec3 pos, vec3 *variety)
{
	pos = pos * 0.0015;
	vec3 v1, v2;
	float height = (
		distorted_height(pos, &v1) +
		distorted_height(pos * 2, &v2)
		) / 2;
	*variety = (v1 + v2) / 2;
	return TERRAIN_AMPLITUDE * height;
}

// Public Functions //

proc_planet * proc_planet_new(float radius, height_map_func height, vec3 color_family)
{
	proc_planet *p = malloc(sizeof(proc_planet));
	p->radius = radius;
	p->color_family = color_family; //TODO: Will eventually choose from a palette of good color combos.
	p->noise_radius = radius/1000; //TODO: Determine the largest reasonable noise radius, map input radius to a good range.
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

	//TODO: Make this based on the position of the planet or something.
	open_simplex_noise(planet_seed++, &p->osnctx);

	return p;
}

void proc_planet_free(proc_planet *p)
{
	open_simplex_noise_free(p->osnctx);
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
	//TODO: Check the distance here and draw an imposter instead of the whole planet if it's far enough.

	for (int i = 0; i < NUM_ICOSPHERE_FACES; i++) {
		terrain_tree_gen(p->tiles[i], proc_planet_split_depth, (terrain_tree_split_fn)tri_tile_split, &context);
		terrain_tree_drawlist_new(p->tiles[i], proc_planet_split_depth, &context, list);
		terrain_tree_prune(p->tiles[i], proc_planet_split_depth, &context, (terrain_tree_free_fn)free_tri_tile);
	}

	//printf("%d\n", context.visited);
}

struct proc_planet_tile_raycast_context {
	vec3 pos;
	space_sector sec;
};

bool proc_planet_tile_raycast(terrain_tree_node *tree, void *context)
{
	struct proc_planet_tile_raycast_context *ctx = (struct proc_planet_tile_raycast_context *)context;
	tri_tile *t = tree_tile(tree);
	vec3 intersection;
	vec3 local_start = space_sector_position_relative_to_sector(ctx->pos, ctx->sec, t->sector);
	vec3 local_end = space_sector_position_relative_to_sector((vec3){0, 0, 0}, (space_sector){0, 0, 0}, t->sector);
	//Flip start and end so we don't find tiles from the back of the planet.
	int result = ray_tri_intersect(local_start, local_end, tree_tile(tree)->tile_vertices, &intersection);
	if (result == 1) {
		t->override_col *= (vec3){0.1, 1.0, 1.0};
		printf("Tile intersection found! Tile: %i\n", t->tile_index);
	}
	return result == 1;
}

//Raycast towards the planet center and find the altitude on the deepest terrain tile. O(log(n)) complexity in the number of planet tiles.
float proc_planet_altitude(proc_planet *p, vec3 pos, space_sector sec)
{
	//TODO: Don't loop through everything to find this.
	float smallest_distance = INFINITY;
	struct proc_planet_tile_raycast_context context = {pos, sec};
	for (int i = 0; i < NUM_ICOSPHERE_FACES; i++) {
		tri_tile *t = terrain_tree_find(p->tiles[i], proc_planet_tile_raycast, &context);
		if (t) {
			vec3 local_start = space_sector_position_relative_to_sector(pos, sec, t->sector);
			vec3 local_end = space_sector_position_relative_to_sector((vec3){0, 0, 0}, (space_sector){0, 0, 0}, t->sector);
			float distance = tri_tile_raycast_depth(t, local_start, local_end);
			if (distance < smallest_distance)
				smallest_distance = distance;
		}
	}
	return smallest_distance;
}


