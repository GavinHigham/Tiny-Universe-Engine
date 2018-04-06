#ifndef PROCEDURAL_PLANET_H
#define PROCEDURAL_PLANET_H
#include "glla.h"
//#include "dynamic_terrain_tree.h"
#include "../quadtree.h"
#include "triangular_terrain_tile.h"
#include "../terrain_constants.h"
#include "../math/bpos.h"

/*
MEGA TODO:
Make a table of minerals, with different color properties. Noise functions and things like asteroid impacts,
diffusion-limited aggregation, continental plate drift, star system composition, etc. can affect the deposition
of these minerals on the surface of the planet. In turn, these minerals can affect the color of the surface,
the kind of surface features we'll see, what kind of plants we may see, and even the position of cities (which
could, say, pop up near an ancient asteroid impact that left behind a valuable resource to mine). Do minerals
affect where continent plates form, or do continent plates form first and push minerals around?

Ideas for continent creation:
Place points on sphere, find voronoi cells, fractelize edges, or distort using a noise function.
Start at a random point, random walk until it intersects with itself, make the closed area a continent, repeat.

A simple way to make mountains from that is to add medium-frequency bumpiness at continent borders, with a falloff.
That bumpiness can be done using diffusion-limited aggregation (or some approximation), to get a "mountain range"
looking effect, with some apparent erosion. Ideally I'd have some data structure so that any point on the sphere
can quickly look up its continent, nearby mountain range, local surface features, etc. Noise can then be saved for
the highest-frequency features, for efficiency. Could also use it to distort the vertexes of my "mountain graph",
like with domain warping (could look good, could look bad).
*/

enum {
	PROC_PLANET_MAX_NUM_ELEMENTS = 4,
	//The more rows, the fewer draw calls, and the fewer primitive restart indices hurting memory locality.
	//The fewer rows, the fewer overall vertices, and the better overall culling efficiency.
	//If I keep it as a power-of-two, I can avoid using spherical linear interpolation, and it will be faster.
	PROC_PLANET_NUM_TILE_ROWS = 64,
	PROC_PLANET_TILE_PIXELS_PER_TRI = 5,
	PROC_PLANET_TILE_MAX_SUBDIVISIONS = 7, //TODO(Gavin): Choose a number that sets the surface resolution to a nice number.
};

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
	int elements[PROC_PLANET_MAX_NUM_ELEMENTS];
	int num_elements;
	quadtree_node *tiles[NUM_ICOSPHERE_FACES];
	height_map_func height;
	float ms_per_tile_gen, ms_per_tile_buffer;
} proc_planet;

struct planet_terrain_context {
	int splits_left;
	int splits_max;
	bpos cam_pos;
	proc_planet *planet;
	int visited;
	tri_tile **tiles;
	int num_tiles;
	int max_tiles;
	bool excess_tiles;
};

int proc_planet_init();
void proc_planet_deinit();
proc_planet * proc_planet_new(float radius, height_map_func height, int *elements, int num_elements);

void proc_planet_free(proc_planet *p);
int proc_planet_drawlist(proc_planet *p, tri_tile **tiles, int max_tiles, bpos cam_pos);
void proc_planet_draw(amat4 eye_frame, float proj_view_mat[16], proc_planet *planets[], bpos planet_positions[], int num_planets);
float proc_planet_height(vec3 pos, vec3 *variety);

//Raycast towards the planet center and find the altitude on the deepest terrain tile. O(log(n)) complexity in the number of planet tiles.
float proc_planet_altitude(proc_planet *p, bpos start, bpos *intersection);

#endif
