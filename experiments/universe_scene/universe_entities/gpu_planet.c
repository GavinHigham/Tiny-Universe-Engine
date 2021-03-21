#include "experiments/universe_scene/universe_entities/gpu_planet.h"
#include "experiments/universe_scene/universe_ecs.h"
#include "experiments/universe_scene/universe_components.h"

extern ecs_ctx *puniverse_ecs_ctx;
extern struct entity_ctypes universe_ecs_ctypes;
#define E puniverse_ecs_ctx
#define ctypes universe_ecs_ctypes

#include "configuration/lua_configuration.h"
#include "datastructures/quadtree.h"
#include "debug_graphics.h"
#include "draw.h"
#include "drawf.h" //For draw planets
#include "effects.h"
#include "space/element.h"
#include "glsw/glsw.h"
#include "glsw_shaders.h"
#include "graphics.h"
#include "input_event.h" //For controller hotkeys
#include "macros.h"
#include "math/geometry.h"
#include "math/utility.h"
#include "open-simplex-noise-in-c/open-simplex-noise.h"
#include "gpu_planet.h"
#include "shader_utils.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct {
	GLuint vao;
	bool is_init;
} gpu_planets = {0, 0};

extern bpos_origin eye_sector;
extern bpos_origin tri_sector;
extern float log_depth_intermediate_factor;
//TODO(Gavin): Expose these through an API or pass them into the draw function.
extern vec3 sun_position;
extern vec3 sun_color;
 
//Adapted from http://www.glprogramming.com/red/chapter02.html
static const float x = 0.525731112119133606;
static const float z = 0.850650808352039932;
static const vec3 ico_v[] = {    
	{-x,  0, z}, { x,  0,  z}, {-x, 0, -z}, { x, 0, -z}, {0,  z, x}, { 0,  z, -x},
	{ 0, -z, x}, { 0, -z, -x}, { z, x,  0}, {-z, x,  0}, {z, -x, 0}, {-z, -x,  0}
};

static const int ico_i[] = {
	1,0,4,   9,4,0,   9,5,4,   8,4,5,   4,8,1,  10,1,8,  10,8,3,  5,3,8,   3,5,2,  9,2,5,  
	3,7,10,  6,10,7,  7,11,6,  0,6,11,  0,1,6,  10,6,1,  0,11,9,  2,9,11,  3,2,7,  11,7,2,
};

//Texture coordinates for each vertex of a face. Pairs of faces share a texture.
static const float ico_tx[] = {
	0.0,0.0, 1.0,0.0, 0.0,1.0,  1.0,1.0, 0.0,1.0, 1.0,0.0,  0.0,0.0, 1.0,0.0, 0.0,1.0,  1.0,1.0, 0.0,1.0, 1.0,0.0, 
	0.0,0.0, 1.0,0.0, 0.0,1.0,  1.0,1.0, 0.0,1.0, 1.0,0.0,  0.0,0.0, 1.0,0.0, 0.0,1.0,  1.0,1.0, 0.0,1.0, 1.0,0.0, 
	0.0,0.0, 1.0,0.0, 0.0,1.0,  1.0,1.0, 0.0,1.0, 1.0,0.0,  0.0,0.0, 1.0,0.0, 0.0,1.0,  1.0,1.0, 0.0,1.0, 1.0,0.0, 
	0.0,0.0, 1.0,0.0, 0.0,1.0,  1.0,1.0, 0.0,1.0, 1.0,0.0,  0.0,0.0, 1.0,0.0, 0.0,1.0,  1.0,1.0, 0.0,1.0, 1.0,0.0, 
	0.0,0.0, 1.0,0.0, 0.0,1.0,  1.0,1.0, 0.0,1.0, 1.0,0.0,  0.0,0.0, 1.0,0.0, 0.0,1.0,  1.0,1.0, 0.0,1.0, 1.0,0.0,
};

//In my debug view, these colors are applied to tiles based on the number of times they've been split.
vec3 gpu_planet_primary_color_by_depth[] = {
	{1.0, 0.0, 0.0}, //Red
	{0.5, 0.5, 0.0}, //Yellow
	{0.0, 1.0, 0.0}, //Green
	{0.0, 0.5, 0.5}, //Cyan
	{0.0, 0.0, 1.0}, //Blue
	{0.5, 0.0, 0.5}, //Purple
};

const vec3 gpu_planet_up = (vec3){z/3, (z+z+x)/3, 0}; //Centroid of ico_i[3]
//Originally made to get rid of the black dot at the poles. Weirdly, they disappeared when I tilted the axis.
//Keep around and use on the tiles (and descendent tiles) of the poles to get rid of the black dots, should they reappear.
//const vec3 gpu_planet_not_up = (vec3){-(z+z+x)/3, z/3, 0};
extern float screen_width;
extern struct osn_context *osnctx;

/* Rows */

#define NUM_ROWS GPU_PLANET_NUM_TILE_ROWS
#define VERTS_PER_ROW(rows) ((rows+2)*(rows+1)/2)
static int rows = NUM_ROWS;

/* Lua Config */
extern lua_State *L;

/* OpenGL Variables */

static GLuint ROWS;
static GLuint SHADER, VAO, MM, MVPM, TEXSCALE, PRADIUS, LCOL, LPOS, EYEPOS, PORIGIN, VBO, INBO;
static GLint SAMPLER0, VLERPS_ATTR;
static GLint POS_ATTR[3] = {1,2,3}, TX_ATTR[3] = {4,5,6};
static GLuint gpu_planet_tx = 0;

/* Instance Attribute Data */

struct instance_attributes {
	float pos[9];
	float tx[6];
};
extern int PRIMITIVE_RESTART_INDEX;

// Static Functions //

static tri_tile * tree_tile(quadtree_node *tree)
{
	return (tri_tile *)tree->data;
}

static int splits_per_distance(float distance, float scale)
{
	return fmax(fmin(log2(scale/distance), GPU_PLANET_TILE_MAX_SUBDIVISIONS), 0);
}

static void tri_tile_split(tri_tile *t, tri_tile *out[DEFAULT_NUM_TRI_TILE_DIVS]);

static bool above_horizon(tri_tile *tile, int depth, struct gpu_planet_terrain_context ctx)
{
	ctx.cam_pos.offset = bpos_remap(ctx.cam_pos, (bpos_origin){0});
	vec3 tile_pos = bpos_remap((bpos){tile->centroid, tile->offset}, (bpos_origin){0});
	float altitude = fmax(vec3_mag(ctx.cam_pos.offset) - ctx.planet->radius, 0);
	return nes30_buttons[INPUT_BUTTON_START] || 
		distance_to_horizon(ctx.planet->radius, altitude) > (vec3_dist(ctx.cam_pos.offset, tile_pos) - tile->radius);
}

struct tile_cull_data {
	vec3 tile_pos;
	float tile_dist, altitude;
};

//TODO: See if I can factor out depth somehow.
static int gpu_planet_subdiv_depth(gpu_planet *planet, tri_tile *tile, int depth, bpos cam_pos)
{
	//Convert camera and tile position to planet-coordinates.
	//These calculations might hit the limits of floating-point precision if the planet is really large.
	cam_pos.offset = bpos_remap(cam_pos, (bpos_origin){0});
	vec3 tile_pos = bpos_remap((bpos){tile->centroid, tile->offset}, (bpos_origin){0});
	vec3 surface_pos = cam_pos.offset * planet->radius/vec3_mag(cam_pos.offset);

	float altitude = vec3_mag(cam_pos.offset) - planet->radius;
	float tile_dist = vec3_dist(surface_pos, tile_pos) - tile->radius;
	float subdiv_dist = fmax(altitude, tile_dist);
	float scale_factor = (screen_width * planet->edge_len) / (2 * GPU_PLANET_TILE_PIXELS_PER_TRI * GPU_PLANET_NUM_TILE_ROWS);

	//Maybe I can handle this when preparing the drawlist, instead?
	//TODO(Gavin): Some of this effort is duplicated in "above_horizon". See if I can refactor.
	float horizon_dist = distance_to_horizon(planet->radius, fmax(altitude, 0));
	float direct_dist = vec3_dist(cam_pos.offset, tile_pos) - tile->radius;
	if (horizon_dist < direct_dist && !nes30_buttons[INPUT_BUTTON_START])
		return 0; //Tiles beyond the horizon should not be split.

	return splits_per_distance(subdiv_dist, scale_factor);
}

static bool gpu_planet_split_visit(quadtree_node *node, void *context)
{
	struct gpu_planet_terrain_context *ctx = (struct gpu_planet_terrain_context *)context;
	tri_tile *tile = tree_tile(node);
	ctx->visited++;

	//Cap the number of subdivisions per whole-tree traversal (per frame essentially)
	if (node->depth == 0)
		ctx->splits_left = ctx->splits_max;

	int depth = gpu_planet_subdiv_depth(ctx->planet, tile, node->depth, ctx->cam_pos);

	if (depth > node->depth && !quadtree_node_has_children(node) && ctx->splits_left > 0) {
		tri_tile *new_tiles[DEFAULT_NUM_TRI_TILE_DIVS];
		tri_tile_split(tile, new_tiles);
		quadtree_node_add_children(node, (void **)new_tiles);
		ctx->splits_left--;
	}

	if (nes30_buttons[INPUT_BUTTON_START])
		tile->override_col = gpu_planet_primary_color_by_depth[depth % LENGTH(gpu_planet_primary_color_by_depth)];
	else
		tile->override_col = (vec3){1, 1, 1};

	return depth > node->depth;
}

static float noise3(vec3 pos)
{
	return (1+open_simplex_noise3(osnctx, pos.x, pos.y, pos.z))/2;
}

static tri_tile * gpu_planet_vertices_and_normals(struct element_properties *elements, int num_elements, tri_tile *t, height_map_func height, vec3 planet_pos, float noise_radius, float planet_radius, float amplitude)
{
	//vec3 brownish = {0.30, .27, 0.21};
	//vec3 whiteish = {0.96, .94, 0.96};
	//vec3 orangeish = (vec3){255, 181, 112} / 255;

	//TODO: Check this value or make it empirical somehow.
	float epsilon = vec3_dist(t->big_vertices[0].position, t->big_vertices[2].position) * (noise_radius / planet_radius / t->num_rows / 5);

	for (int i = 0; i < t->num_vertices; i++) {
		//Points towards vertex from planet origin
		vec3 pos = t->mesh[i].position - planet_pos;
		float m = vec3_mag(pos);
		//Point at the surface of our simulated smaller planet.
		vec3 noise_surface = pos * (noise_radius/m);

		//Basis vectors
		//TODO: Use a different "up" vector on the poles.
		vec3 x = vec3_normalize(vec3_cross(gpu_planet_up, pos));
		vec3 z = vec3_normalize(vec3_cross(pos, x));
		vec3 y = vec3_normalize(pos);

		//Create two points, scootched out along the basis vectors.
		vec3 pos1 = x * epsilon + noise_surface;
		vec3 pos2 = z * epsilon + noise_surface;
		vec3 p0 = noise_surface * 0.00005, p1 = pos1 * 0.00005, p2 = pos2 * 0.00005;

		vec3 cmy, sum_cmy = {0,0,0}; float k, sum_k = 0, sum_h = 0;
		double ys[3] = {0};
		//TODO(Gavin): Using noise gradient, can make this ~3 times faster (no finite differences).
		for (int j = 0; j < num_elements; j++) {
			float offset = 4529 * j;//random number
			float scale = pow(2, j*2)*2;
			vec3 turb = {ys[0], ys[1], ys[2]};
			double h = noise3(turb + p0 * scale + offset);
			rgb_to_cmyk(elements[j].color, &cmy, &k);
			sum_h += h;
			sum_cmy += cmy*h;
			sum_k += k*h;
			ys[0] += scale * h;
			ys[1] += scale * noise3(turb + p1 * scale + offset);
			ys[2] += scale * noise3(turb + p2 * scale + offset);
		}
		cmy = sum_cmy / sum_h; k = sum_k / sum_h;
		cmyk_to_rgb(cmy, k, &t->mesh[i].color);
		t->mesh[i].color /= 255;

		pos1          = y * amplitude * ys[1] + pos1;
		pos2          = y * amplitude * ys[2] + pos2;
		noise_surface = y * amplitude * ys[0] + noise_surface;

		//Compute the normal.
		t->mesh[i].normal = vec3_normalize(vec3_cross(pos1 - noise_surface, pos2 - noise_surface));
		//Compute the new surface position.
		//Scale back up to planet size.
		t->mesh[i].position = noise_surface * (m/noise_radius) + planet_pos;
	}

	return t;
}

static void gpu_planet_finishing_touches(tri_tile *t, void *finishing_touches_context)
{
	//Retrieve tile's planet from finishing_touches_context.
	gpu_planet *p = (gpu_planet *)finishing_touches_context;
	vec3 planet_pos = bpos_remap((bpos){0}, t->offset);

	//Curve the tile around planet by normalizing each vertex's distance to the planet and scaling by planet radius.
	for (int i = 0; i < t->num_vertices; i++) {
		vec3 d = t->mesh[i].position - planet_pos;
		t->mesh[i].position = d * p->radius/vec3_mag(d) + planet_pos;
	}

	//Apply perturbations to the surface and calculate normals.
	//Since noise doesn't compute well on huge planets, noise is calculated on a simulated smaller planet and scaled up.
	struct element_properties props[p->num_elements];
	for (int i = 0; i < p->num_elements; i++)
		props[i] = element_get_properties(p->elements[i]);
	gpu_planet_vertices_and_normals(props, p->num_elements, t, p->height, planet_pos, p->noise_radius, p->radius, p->amplitude);
}

static void tri_tile_split(tri_tile *t, tri_tile *out[DEFAULT_NUM_TRI_TILE_DIVS])
{
	struct tri_tile_big_vertex new_vertices[] = {
		tri_tile_get_big_vert_average(t, 0, 1),
		tri_tile_get_big_vert_average(t, 0, 2),
		tri_tile_get_big_vert_average(t, 1, 2)
	};

	gpu_planet *planet = (gpu_planet *)t->finishing_touches_context;
	vec3 planet_pos = bpos_remap((bpos){0}, t->offset);
	for (int i = 0; i < 3; i++) {
		vec3 d = new_vertices[i].position - planet_pos;
		new_vertices[i].position = d * planet->radius/vec3_mag(d) + planet_pos;
	}

	struct tri_tile_big_vertex new_tile_vertices[DEFAULT_NUM_TRI_TILE_DIVS][3] = {
		{t->big_vertices[0], new_vertices[0],    new_vertices[1]},
		{new_vertices[0],    t->big_vertices[1], new_vertices[2]},
		{new_vertices[0],    new_vertices[2],    new_vertices[1]},
		{new_vertices[1],    new_vertices[2],    t->big_vertices[2]}
	};

	for (int i = 0; i < DEFAULT_NUM_TRI_TILE_DIVS; i++) {
		out[i] = tri_tile_new(new_tile_vertices[i]);
		tri_tile_init(out[i], t->offset, GPU_PLANET_NUM_TILE_ROWS, t->finishing_touches, t->finishing_touches_context);
		//tri_tile_buffer(out[i]);
	}

	printf("Dividing %p.\n", t);
}

static float fbm(vec3 p)
{
	return open_simplex_noise3(osnctx, p.x, p.y, p.z);
}

static float distorted_height(vec3 pos, vec3 *variety)
{
	vec3 a = {1.4, 1.08, 1.3};
	vec3 b = {3.8, 7.9, 2.1};

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

float gpu_planet_height(vec3 pos, vec3 *variety)
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

//Declaring these here for now, until I move them to a more permanent location.
int get_tri_lerp_vals(float *lerps, int num_rows);
GLuint load_gl_texture(char *path);
int gpu_planet_init()
{
	if (!gpu_planets.is_init) {
		glGenVertexArrays(1, &gpu_planets.vao);
		glBindVertexArray(gpu_planets.vao);
		glEnableVertexAttribArray(effects.forward.vPos);
		glEnableVertexAttribArray(effects.forward.vNormal);
		glEnableVertexAttribArray(effects.forward.vColor);
		glVertexAttribPointer(effects.forward.vPos,    3, GL_FLOAT, GL_FALSE, 0, NULL);
		glVertexAttribPointer(effects.forward.vNormal, 3, GL_FLOAT, GL_FALSE, 0, NULL);
		glVertexAttribPointer(effects.forward.vColor,  3, GL_FLOAT, GL_FALSE, 0, NULL);
		gpu_planets.is_init = true;
	}

	rows = getglob(L, "num_tile_rows", NUM_ROWS);

	/* Triangle patch setup */
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	/* Shader initialization */
	glswInit();
	glswSetPath("shaders/glsw/", ".glsl");
	glswAddDirectiveToken("glsl330", "#version 330");

	GLuint shader[] = {
		glsw_shader_from_keys(GL_VERTEX_SHADER, "versions.glsl330", "common.noise.GL33", "proc_planet.vertex.GL33"),
		glsw_shader_from_keys(GL_FRAGMENT_SHADER, "versions.glsl330", "common.noise.GL33", "proc_planet.fragment.GL33", "common.lighting"),
	};
	SHADER = glsw_new_shader_program(shader, LENGTH(shader));
	glswShutdown();

	if (!SHADER) {
		gpu_planet_deinit();
		return -1;
	}

	/* Retrieve uniform variable handles */

	MM       = glGetUniformLocation(SHADER, "model_matrix");
	MVPM     = glGetUniformLocation(SHADER, "model_view_projection_matrix");
	SAMPLER0 = glGetUniformLocation(SHADER, "diffuse_tx");
	ROWS     = glGetUniformLocation(SHADER, "rows");
	TEXSCALE = glGetUniformLocation(SHADER, "tex_scale");
	PRADIUS  = glGetUniformLocation(SHADER, "planet_radius");
	LPOS     = glGetUniformLocation(SHADER, "light_pos");
	LCOL     = glGetUniformLocation(SHADER, "light_col");
	EYEPOS   = glGetUniformLocation(SHADER, "eye_pos");
	PORIGIN  = glGetUniformLocation(SHADER, "planet_origin");
	checkErrors("After getting uniform handles");

	/* Vertex data */

	float tri_lerps[2 * VERTS_PER_ROW(rows)];
	get_tri_lerp_vals(tri_lerps, rows);
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(tri_lerps), tri_lerps, GL_STATIC_DRAW);

	glGenBuffers(1, &INBO);
	glBindBuffer(GL_ARRAY_BUFFER, INBO);
	checkErrors("After gen indexed array buffer");

	/* Vertex attributes */

	int attr_div = 1;
	for (int i = 0; i < 3; i++)
	{
		glEnableVertexAttribArray(POS_ATTR[i]);
		glVertexAttribPointer(POS_ATTR[i], 3, GL_FLOAT, GL_FALSE,
			sizeof(struct instance_attributes), (void *)(offsetof(struct instance_attributes, pos) + i*3*sizeof(float)));
		glVertexAttribDivisor(POS_ATTR[i], attr_div);
		checkErrors("After attr divisor for pos");
	}

	for (int i = 0; i < 3; i++)
	{
		glEnableVertexAttribArray(TX_ATTR[i]);
		glVertexAttribPointer(TX_ATTR[i], 2, GL_FLOAT, GL_FALSE,
			sizeof(struct instance_attributes), (void *)(offsetof(struct instance_attributes, tx) + i*2*sizeof(float)));
		glVertexAttribDivisor(TX_ATTR[i], attr_div);
		checkErrors("After attr divisor for attr");
	}

	VLERPS_ATTR = 7;
	glEnableVertexAttribArray(VLERPS_ATTR);

	checkErrors("After setting attrib divisors");
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	checkErrors("After binding VBO");
	glVertexAttribPointer(VLERPS_ATTR, 2, GL_FLOAT, GL_FALSE, 0, 0);
	checkErrors("After setting VBO attrib pointer");

	/* Misc. OpenGL bits */

	glClearDepth(1);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDepthFunc(GL_LESS);
	glEnable(GL_PRIMITIVE_RESTART);
	glPrimitiveRestartIndex(PRIMITIVE_RESTART_INDEX);
	glBindVertexArray(0);

	/* For rotating the icosahedron */
	//SDL_SetRelativeMouseMode(true);

	char texture_path[gettmpglobstr(L, "proctri_tex", "grass.png", NULL)];
	                  gettmpglobstr(L, "proctri_tex", "grass.png", texture_path);
	gpu_planet_tx = load_gl_texture(texture_path);
	glUseProgram(SHADER);
	glUniform1f(TEXSCALE, getglob(L, "tex_scale", 1.0));
	glUniform1f(glGetUniformLocation(SHADER, "log_depth_intermediate_factor"), log_depth_intermediate_factor);
	glUseProgram(0);

	return 0;
}

void gpu_planet_deinit()
{
	if (gpu_planets.is_init) {
		gpu_planets.is_init = false;
		glDeleteVertexArrays(1, &gpu_planets.vao);
	}

	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteProgram(SHADER);
}

gpu_planet * gpu_planet_new(float radius, height_map_func height, int *elements, int num_elements)
{
	gpu_planet *p = malloc(sizeof(gpu_planet));
	*p = (gpu_planet) {
		.radius       = radius,
		.num_elements = num_elements,
		.noise_radius = radius/1000, //TODO: Determine the largest reasonable noise radius, map input radius to a good range.
		.amplitude = TERRAIN_AMPLITUDE,
		.edge_len = radius / sin(2.0*M_PI/5.0),
		.height = height
	};
	for (int i = 0; i < num_elements; i++)
		p->elements[i] = elements[i];
	printf("Edge len: %f\n", p->edge_len);

	uint32_t ticks = SDL_GetTicks();

	//Initialize the planet terrain
	for (int i = 0; i < NUM_ICOSPHERE_FACES; i++) {
		struct tri_tile_big_vertex verts[3];
		for (int j = 0; j < 3; j++)
			verts[j] = (struct tri_tile_big_vertex){ico_v[ico_i[3*i+j]] * radius, {ico_tx[(6*i)+(2*j)], ico_tx[(6*i)+(2*j)+1]}};

		p->tiles[i] = quadtree_new(tri_tile_new(verts), 0);
		//Initialize tile with verts expressed relative to p->sector.
		tri_tile_init((tri_tile *)p->tiles[i]->data, (bpos_origin){0, 0, 0}, GPU_PLANET_NUM_TILE_ROWS, &gpu_planet_finishing_touches, p);
	}

	uint32_t ticks2 = SDL_GetTicks();

	for (int i = 0; i < NUM_ICOSPHERE_FACES; i++)
		tri_tile_buffer(p->tiles[i]->data);

	uint32_t ticks3 = SDL_GetTicks();

	p->ms_per_tile_gen    = (ticks2 - ticks)  / (float)NUM_ICOSPHERE_FACES;
	p->ms_per_tile_buffer = (ticks3 - ticks2) / (float)NUM_ICOSPHERE_FACES;
	printf("Planet took %f, %f ms to generate.\n", p->ms_per_tile_gen, p->ms_per_tile_buffer);

	return p;
}

void gpu_planet_free(gpu_planet *p)
{
	for (int i = 0; i < NUM_ICOSPHERE_FACES; i++)
		quadtree_free(p->tiles[i], (quadtree_free_fn)tri_tile_free);
	free(p);
}

static bool gpu_planet_drawlist_visit(quadtree_node *node, void *context)
{
	struct gpu_planet_terrain_context *ctx = (struct gpu_planet_terrain_context *)context;
	tri_tile *tile = tree_tile(node);
	int depth = gpu_planet_subdiv_depth(ctx->planet, tile, node->depth, ctx->cam_pos);

	if (depth == node->depth || !quadtree_node_has_children(node)) {
		if (ctx->num_tiles < ctx->max_tiles && above_horizon(tile, depth, *ctx))
			ctx->tiles[ctx->num_tiles++] = tile;
		// else
		// 	ctx->excess_tiles = true;
	}

	return depth > node->depth;
}

int gpu_planet_drawlist(gpu_planet *p, tri_tile **tiles, int max_tiles, bpos cam_pos)
{
	struct gpu_planet_terrain_context context = {
		.splits_max = fmax(15.0 / (4 * (p->ms_per_tile_gen + p->ms_per_tile_buffer)), 1), //Total number of splits for this call of drawlist.
		.cam_pos = cam_pos,
		.planet = p,
		.tiles = tiles,
		.max_tiles = max_tiles
	};
	//TODO: Check the distance here and draw an imposter instead of the whole planet if it's far enough.

	for (int i = 0; i < NUM_ICOSPHERE_FACES; i++) {
		quadtree_preorder_visit(p->tiles[i], gpu_planet_split_visit, &context);
		quadtree_preorder_visit(p->tiles[i], gpu_planet_drawlist_visit, &context);
		if (context.excess_tiles)
			printf("Excess tiles.\n");
		//TODO: Find a good way to implement prune with hysteresis.
		//terrain_tree_prune(p->tiles[i], gpu_planet_split_depth, &context, (terrain_tree_free_fn)tri_tile_free);
	}

	//printf("Num tiles drawn:%d\n", context.visited);
	return context.num_tiles;
}

//Duplicate from draw.c for now
void gp_prep_matrices(amat4 model_matrix, GLfloat pvm[16], GLfloat mm[16], GLfloat mvpm[16], GLfloat mvnm[16])
{
	amat4_to_array(model_matrix, mm);
	amat4_buf_mult(pvm, mm, mvpm);
	mat3_vec3_to_array(mat3_transp(model_matrix.a), (vec3){0, 0, 0}, mvnm);
}

bool gpu_planet_point_in_viewport(float *mvpm, float *vpos, float *out)
{
	float vpos_buf[4] = {vpos[0], vpos[1], vpos[2], 1};
	float gl_Position[4];
	bool tmp = true;
	amat4_buf_multpoint(mvpm, vpos_buf, gl_Position);//vec4(p,1);//vec4(normalize(position) * pow(tex.r-tex.g, 0.03), 1);
	gl_Position[2] = (log2(fmax(/*1e-6*/0.000001, 1.0 + gl_Position[2])) * log_depth_intermediate_factor - 1.0) * gl_Position[3];
	for (int i = 0; i < 3; i++) {
		gl_Position[i] /= gl_Position[3];
		if (gl_Position[i] > 1.0 || gl_Position[i] < -1.0) {
			tmp = false;
			break;
		}
	}

	if (out)
		memcpy(out, gl_Position, 3 * sizeof(float));

	return tmp;;
}

void gpu_planet_draw(amat4 eye_frame, float proj_view_mat[16], gpu_planet *planets[], bpos planet_positions[], int num_planets)
{
	//Create a list of planet tiles to draw.
	amat4 tri_frame  = {.a = MAT3_IDENT, .t = {0, 0, 0}};
	int drawlist_max = 5000 * num_planets; //TODO(Gavin): Get a good estimate of this from actual number of runtime tiles.
	tri_tile *drawlist[drawlist_max] __attribute__((aligned(64)));
	//Index for the start of each planet's list of tiles, so I can assign uniforms per-planet.
	int planet_tiles_start[num_planets + 1] __attribute__((aligned(64))); //Compiler bug!
	int drawlist_count = 0;
	//Collect tiles for each planet, store in drawlist.
	for (int i = 0; i < num_planets; i++) {
		bpos pos = {eye_frame.t - planet_positions[i].offset, eye_sector - planet_positions[i].origin};
		planet_tiles_start[i] = drawlist_count;
		int planet_count = gpu_planet_drawlist(planets[i], drawlist + drawlist_count, drawlist_max - drawlist_count, pos);
		drawlist_count += planet_count;
		if (key_state[SDL_SCANCODE_2])
			printf("Planet %2i drawing %10i tiles this frame.\n", i, planet_count);
	}
	planet_tiles_start[num_planets] = drawlist_count;

	if (getglobbool(L, "gpu_tiles", false) != key_state[SDL_SCANCODE_3]) {
		struct instance_attributes planet_tile_data[drawlist_count] __attribute__((aligned(64))); //Compiler bug!

		for (int i = 0; i < num_planets; i++) {
			for (int j = planet_tiles_start[i]; j < planet_tiles_start[i+1]; j++) {
				tri_tile *t = drawlist[j];

				//For each tile, convert "big vertex" positions to camera-space (or "current sector" space)
				vec3 big_vert_offset = bpos_remap((bpos){planet_positions[i].offset, planet_positions[i].origin + t->offset}, eye_sector);
				for (int k = 0; k < 3; k++) {
					struct tri_tile_big_vertex tmp = drawlist[j]->big_vertices[k];
					tmp.position += big_vert_offset;
					memcpy(&planet_tile_data[j].pos[3*k], &tmp.position, 3*sizeof(float));
					memcpy(&planet_tile_data[j].tx[2*k],  &tmp.tx,       2*sizeof(float));
				}
			}
		}

		//To debug GPU tile alignment.
		GLfloat mm[16];
		amat4 gpu_model_matrix = {MAT3_IDENT, eye_frame.t};
		amat4_to_array(gpu_model_matrix, mm);

		glBindVertexArray(VAO);
		glUseProgram(SHADER);

		glUniform3f(LCOL, VEC3_COORDS(sun_color));
		glUniform3f(LPOS, VEC3_COORDS(sun_position));
		glUniform3f(EYEPOS, VEC3_COORDS(eye_frame.t));
		checkErrors("After lighting uniforms");
		
		//This should be per-planet. Maybe pack into a vertex attribute? Otherwise each planet can be a separate draw call.
		glUniform3f(PORIGIN, VEC3_COORDS(bpos_remap(planet_positions[0], eye_sector)));
		glUniform1f(PRADIUS, planets[0]->radius);
		glUniformMatrix4fv(MM, 1, true, mm);
		glUniformMatrix4fv(MVPM, 1, true, proj_view_mat);
		checkErrors("After uniforms");
		glBindBuffer(GL_ARRAY_BUFFER, INBO);
		checkErrors("After bind INBO");
		//glBufferData(GL_ARRAY_BUFFER, sizeof(test_data), test_data, GL_DYNAMIC_DRAW);
		glBufferData(GL_ARRAY_BUFFER, sizeof(planet_tile_data), planet_tile_data, GL_DYNAMIC_DRAW);
		checkErrors("After upload indexed array data");
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, gpu_planet_tx);
		glUniform1i(SAMPLER0, 0);
		glUniform1i(ROWS, rows);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, get_shared_tri_tile_indices_buffer_object(rows));
		checkErrors("After binding INBO");
		glDrawElementsInstanced(GL_TRIANGLE_STRIP, num_tri_tile_indices(rows), GL_UNSIGNED_INT, NULL, drawlist_count);
		checkErrors("After instanced draw");

		// if (key_state[SDL_SCANCODE_4]) {
		// 	for (int i = 0; i < drawlist_count; i++) {
		// 		struct instance_attributes *ia = &(planet_tile_data[i]);

		// 		for (int k = 0; k < 3; k++) {
		// 			float gpu_clip_coords[3];
		// 			bool gpu_in_viewport = gpu_planet_point_in_viewport(gpu_mvpm, ia->pos + 3*k, gpu_clip_coords);
		// 			uint32_t hash = float3_hash(gpu_clip_coords, 18) % 230 + 1;

		// 			printf(ANSI_NUMERIC_FCOLOR "%s%2i: % f, % f, % f" ANSI_COLOR_RESET "\n",
		// 				hash,
		// 				gpu_in_viewport ? "" : "~", i, gpu_clip_coords[0], gpu_clip_coords[1], gpu_clip_coords[2]);
		// 		}
		// 	}
		// }

	} else {
		glUseProgram(effects.forward.handle);
		for (int i = 0; i < num_planets; i++) {
			for (int j = planet_tiles_start[i]; j < planet_tiles_start[i+1]; j++) {
				tri_tile *t = drawlist[j];
				bpos tile_pos = planet_positions[i];
				tile_pos.offset = bpos_remap((bpos){tile_pos.offset, tile_pos.origin + t->offset}, eye_sector);
				amat4 tile_frame = {tri_frame.a, tile_pos.offset};
				GLfloat mm[16], mvpm[16], mvnm[16];

				glBindVertexArray(t->vao);

				if (!t->buffered) //Last resort "BUFFER RIGHT NOW", will cause hiccups.
					tri_tile_buffer(t);
				
				glUniform3fv(effects.forward.override_col, 1, (float *)&t->override_col);
				gp_prep_matrices(tile_frame, proj_view_mat, mm, mvpm, mvnm);

				glUniformMatrix4fv(effects.forward.model_matrix,                 1, true, mm);
				glUniformMatrix4fv(effects.forward.model_view_projection_matrix, 1, true, mvpm);
				glUniformMatrix4fv(effects.forward.model_view_normal_matrix,     1, true, mvnm);

				glDrawElements(GL_TRIANGLE_STRIP, t->num_indices, GL_UNSIGNED_INT, NULL);
				checkErrors("Drew a planet tile");

				//Debug printing to figure out why GPU tiles aren't aligned right.
				// if (key_state[SDL_SCANCODE_4]) {
				// 	struct instance_attributes *ia = &(planet_tile_data[j]);

				// 	for (int k = 0; k < 3; k++) {
				// 		float clip_coords[3], gpu_clip_coords[3];
				// 		vec3 tmp = t->big_vertices[k].position;
				// 		float vpos[3] = {tmp.x, tmp.y, tmp.z};

				// 		bool in_viewport     = gpu_planet_point_in_viewport(mvpm, vpos, clip_coords);
				// 		bool gpu_in_viewport = gpu_planet_point_in_viewport(gpu_mvpm, ia->pos + 3*k, gpu_clip_coords);
				// 		uint32_t hashes[2] = {float3_hash(clip_coords, 18) % 230 + 1, float3_hash(gpu_clip_coords, 18) % 230 + 1};
				// 		//vec3 colors[] = {color_from_float3(clip_coords, 10), color_from_float3(gpu_clip_coords, 10)};

				// 		printf(ANSI_NUMERIC_FCOLOR "%s%2i: % f, % f, % f" ANSI_COLOR_RESET "\n",
				// 			hashes[0],
				// 			in_viewport     ? "" : "~", j,     clip_coords[0],     clip_coords[1],     clip_coords[2]);
				// 		printf(ANSI_NUMERIC_FCOLOR "%s%2i: % f, % f, % f" ANSI_COLOR_RESET "\n",
				// 			hashes[1],
				// 			gpu_in_viewport ? "" : "~", j, gpu_clip_coords[0], gpu_clip_coords[1], gpu_clip_coords[2]);
				// 	}
				// }
			}
		}
	}
}

struct gpu_planet_tile_raycast_context {
	//Input
	bpos pos;
	//Output
	tri_tile *intersecting_tile;
	bpos intersection;
};

//Checks if a ray intersects with a planet tile.
bool gpu_planet_tile_raycast(quadtree_node *node, void *context)
{
	struct gpu_planet_tile_raycast_context *ctx = (struct gpu_planet_tile_raycast_context *)context;
	tri_tile *t = tree_tile(node);
	vec3 intersection;
	vec3 local_start = bpos_remap(ctx->pos, t->offset);
	vec3 local_end = bpos_remap((bpos){0}, t->offset);
	vec3 vertices[3] = {t->big_vertices[0].position, t->big_vertices[1].position, t->big_vertices[2].position};
	int result = ray_tri_intersect(local_start, local_end, vertices, &intersection);
	if (result == 1) {
		//t->override_col *= (vec3){0.1, 1.0, 1.0};
		//printf("Tile intersection found! Tile: %i\n", t->tile_index);
		ctx->intersecting_tile = t;
		ctx->intersection.offset = intersection;
		ctx->intersection.origin = t->offset;
	}
	return result == 1;
}

//Raycast towards the planet center and find the altitude on the deepest terrain tile. O(log(n)) complexity in the number of planet tiles.
float gpu_planet_altitude(gpu_planet *p, bpos start, bpos *intersection)
{
	//TODO: Don't loop through everything to find this.
	float smallest_dist = INFINITY;
	float smallest_tile_dist = INFINITY;
	struct gpu_planet_tile_raycast_context context = {
		.pos = start,
		.intersecting_tile = NULL
	};
	for (int i = 0; i < NUM_ICOSPHERE_FACES; i++) {
		//Find the intersecting tiles.
		quadtree_preorder_visit(p->tiles[i], gpu_planet_tile_raycast, &context);
		tri_tile *t = context.intersecting_tile;
		if (t) {
			//Remap ray start relative to tile origin.
			vec3 local_start = bpos_remap(start, t->offset);
			//Remap planet center relative to tile origin.
			vec3 local_end = bpos_remap((bpos){0}, t->offset);
			vec3 local_intersection;
			float dist = tri_tile_raycast_depth(t, local_start, local_end, &local_intersection);

			//Because we'll also find an intersection on the back of the planet, choose smallest distance.
			if (dist < smallest_dist) {
				smallest_dist = dist;
				intersection->offset = local_intersection;
				intersection->origin = t->offset; //Origin relative to planet center.
			}

			float tile_dist = vec3_dist(local_start, t->centroid);
			if (tile_dist < smallest_tile_dist) {
				smallest_tile_dist = tile_dist;
				// *intersection = context.intersection;
			}
		}
		context.intersecting_tile = NULL;
	}
	return smallest_dist;
}

scriptabletemp_callback(entity_gpu_planet_script)
{
	// bpos_origin origin = {0,0,0};
	// target_or_self_origin(eid, &origin);
	// star_box_update(entity_customdrawable(eid)->ctx, origin);
}


customdrawable_callback(entity_gpu_planet_draw)
{
	Camera *c = entity_camera(camera);
	PhysicalTemp *cp = entity_physicaltemp(camera);
	CustomDrawable *cd = entity_customdrawable(self);
	PhysicalTemp *pt = entity_physicaltemp(self);
	assert(c && cd && pt);
	// glUniform1f(effects.star_box.log_depth_intermediate_factor, c->log_depth_intermediate_factor);
	// star_box_draw(entity_customdrawable(self)->ctx, entity_physicaltemp(camera)->origin, c->proj_view_mat);
	// checkErrors("Universe %d", __LINE__);
	gpu_planet *gp = cd->ctx;
	bpos ppos = {.offset = pt->position.t, .origin = pt->origin};
	gpu_planet_draw(cp->position, c->proj_view_mat, &gp, &ppos, 1);
	checkErrors("GPU Planet %d", __LINE__);
}

CD(entity_gpu_planet_destruct)
{
	CustomDrawable *cd = component;
	gpu_planet_free(cd->ctx);
}

uint32_t entity_gpu_planet_new()
{
	uint32_t planet = entity_new();
	entity_add(planet, (Label){.name = "Test GPU Planet", .description = "An entity to test my GPU planet generation."});
	//TODO(Gavin): Salvage / adapt my solar system code to make sense with the ECS
	//Also decide how elements will work with the GPU acceleration
	int num_elements = 4;
	int elements[num_elements];
	element_get_random_set(elements, num_elements);
	gpu_planet *gp = gpu_planet_new(6000000, gpu_planet_height, elements, num_elements);
	entity_add(planet, (PhysicalTemp){.position = {.a = MAT3_IDENT, .t = {0, 0, -6050000}}, .velocity = (amat4)AMAT4_IDENT, .acceleration = (amat4)AMAT4_IDENT});
	entity_add(planet, (ScriptableTemp){.ctx = gp, .script = entity_gpu_planet_script});
	entity_add(planet, (CustomDrawable){.ctx = gp, .draw = entity_gpu_planet_draw, .destruct = entity_gpu_planet_destruct});

	return planet;
}

/*
TODO(Gavin):
- Instead of creating a bunch of VBOs, use the method I developed in proctri_scene
(VBO is parametric coordinates, uniforms define the corners of a triangular tile)
- Create a texture atlas to store tile attributes. UVs can be calculated using math I figured out in the Lua prototype.
Slots can be assigned using an hmempool (backing mempool holds the UV coordinates and corner vertices which ultimately "reserve" a slot)
	(Consider using an ECS with TextureAtlasSlot component? May want to avoid over-generalizing to arbitrary polygonal slots.
	Maybe I don't need this - the quadtree can hold tile information while the hmempool is just for reserved slots.)
- Set up sister hmempool for albedo/roughness/normal etc. shading information. Deterministically matches the layout of the heightmap atlas,
but is many times the resolution. Can use same UV coordinates?!

At 128 rows / tile, (ignoring diagonal between two tiles that share a cell - still need to solve that) I can fit 2048 tiles in a 4096^2 texture.
I believe it took about 3000 tiles to get a good-looking planet with the old technique (or was that just me overprovisioning the array
because I didn't know how many it would take?)
Maybe I can evict parent nodes to save 25%. Evicting non-visible tiles aggressively could get it lower. If I evict parent tiles, I can't
use them to apply lower-frequency detail to smaller tiles, though it would be nice if tiles could be created and destroyed with no dependencies.

- Set up functions to render tiles into the texture atlas. Will read local landmarks per-tile (rivers, mountains, other "decal-like" stuff)
and draw them into the atlas on the GPU.
- Preparing for draw means visiting quadtree, constructing a UBO or VBO-with-glAttribDivisor with all the UV coordinates and corner vertices
- Drawing is as simple as binding the texture, UBO, VBO and calling draw instanced - should be able to draw even multiple planets really easily.
- Think about frustum culling - could be pretty easy to do while walking the tree, just have to be careful to not cull in this case:

              ^
              |
              |
              |
        /|    |
<------/ |---camera frustum
      /  |
a____/   |____b (corner vertices a and b are outside frustum, terrain is not)

- Blending between subdivision levels could be done by having a second set of texture coordinates and sampling a different texture,
or, by placing the previous-detail-level's height in the texture with the current detail level's tile data, allowing for interpolation between
detail levels from a single texture sample. Could also put the delta between the two, for more height resolution (this would need to be fixed-point to
avoid pop-in).

Another idea is for "new" vertices to sample the old vertices adjacent to them, and average their heights to reconstruct what the surface height would
have been (this could be done as part of the draw step and stored to remove the need to do another 2 samples at render time).

In the vertex shader, vertex texture-fetch to get the height, use camera position to determine how to lerp between previous height and current height.
At 0 looks like prev. detail level, at 1 is "full" resolution. Coordination between selected detail level and calculated lerp factor obviates the need
for terrain skirts / overlap.

I'll have to find a solution for doing heightmap lookups CPU-side. I could load heightmaps from the GPU, or re-generate needed height data on the CPU.

Step-by-Step:
1. Create FBO to render to, attach to a camera
2. Create some kind of widget to display FBO
3. Test rendering a triangle into the FBO
4. Test rendering a bunch of triangles into the FBO, using an hmempool to coordinate, storing UVs
5. Make a vertex / fragment shader for terrain
5. Test reading a triangle from FBO in a vertex shader
6. Create icosphere, reading from FBO
7. Add icosphere subdivision
8. Render heightmap into FBO and watch the sphere get all wibbly
9. Read neigbors to reconstruct "parent" height, store for interpolation between detail levels
10. Collision :((((
*/