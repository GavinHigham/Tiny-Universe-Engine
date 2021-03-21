#ifndef gpuplanet_h
#define gpuplanet_h

#include "glla.h"
#include "datastructures/quadtree.h"
#include "space/triangular_terrain_tile.h"
#include "terrain_constants.h"
#include "math/bpos.h"
#include <inttypes.h>

enum {
	GPU_PLANET_MAX_NUM_ELEMENTS = 4,
	//The more rows, the fewer draw calls, and the fewer primitive restart indices hurting memory locality.
	//The fewer rows, the fewer overall vertices, and the better overall culling efficiency.
	//If I keep it as a power-of-two, I can avoid using spherical linear interpolation, and it will be faster.
	GPU_PLANET_NUM_TILE_ROWS = 128,
	GPU_PLANET_TILE_PIXELS_PER_TRI = 5,
	GPU_PLANET_TILE_MAX_SUBDIVISIONS = 7, //TODO(Gavin): Choose a number that sets the surface resolution to a nice number.
};

typedef struct gpu_planet {
	vec3 up;
	vec3 right;
	float radius;
	float noise_radius;
	float amplitude;
	float edge_len;
	//TODO: Consolidate these.
	vec3 colors[3];
	vec3 color_family;
	int elements[GPU_PLANET_MAX_NUM_ELEMENTS];
	int num_elements;
	quadtree_node *tiles[NUM_ICOSPHERE_FACES];
	height_map_func height;
	float ms_per_tile_gen, ms_per_tile_buffer;
} gpu_planet;

struct gpu_planet_terrain_context {
	int splits_left;
	int splits_max;
	bpos cam_pos;
	gpu_planet *planet;
	int visited;
	tri_tile **tiles;
	int num_tiles;
	int max_tiles;
	bool excess_tiles;
};

int gpu_planet_init();
void gpu_planet_deinit();
gpu_planet * gpu_planet_new(float radius, height_map_func height, int *elements, int num_elements);

void gpu_planet_free(gpu_planet *p);
int gpu_planet_drawlist(gpu_planet *p, tri_tile **tiles, int max_tiles, bpos cam_pos);
void gpu_planet_draw(amat4 eye_frame, float proj_view_mat[16], gpu_planet *planets[], bpos planet_positions[], int num_planets);
float gpu_planet_height(vec3 pos, vec3 *variety);

//Raycast towards the planet center and find the altitude on the deepest terrain tile. O(log(n)) complexity in the number of planet tiles.
float gpu_planet_altitude(gpu_planet *p, bpos start, bpos *intersection);

uint32_t entity_gpu_planet_new();

#endif