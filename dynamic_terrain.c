#include <glalgebra.h>
#include <math.h>
#include <assert.h>
#include "procedural_terrain.h"
#include "dynamic_terrain.h"
#include "buflist.h"
#include "render.h"

//Might want to have values for "minimum pixels per triangle" and "maximum pixels per triangle"
//For that I need an equation that relates the number of pixels a triangle occupies with its size and distance.
//Diameter scales linearly with distance
//Triangle size can just be size of a tile / #triangles per row of tile

float pixels_per_tri(float screen_width, float triangle_base, float triangle_distance, int triangle_rows)
{
	return screen_width*triangle_base / (2*triangle_rows*triangle_distance);
}

int subdivisions_per_distance(float distance)
{
	return (int)fmin(log2(pixels_per_tri(screen_width, TRI_BASE_LEN, distance, NUM_TRI_ROWS))/MIN_PIXELS_PER_TRI, MAX_SUBDIVISIONS);
}

DRAWLIST drawlist_prepend(DRAWLIST list, struct terrain *t)
{
	struct drawlist_node *new = malloc(sizeof(struct drawlist_node));
	new->t = t;
	new->next = list;
	return new;
}

void drawlist_free(DRAWLIST list)
{
	if (list) {
		drawlist_free(list->next);
		free(list);
	}
}

PDTNODE new_tree(struct terrain t)
{
	PDTNODE new = malloc(sizeof(struct dynamic_terrain_node));
	new->t = t;
	new->children[0] = NULL;
	return new;
}

void subdivide_tree(PDTNODE tree, vec3 cam_pos, int depth)
{
	assert(tree);
	tree->dist = fmin(
		fmin(vec3_dist(tree->t.pos, cam_pos), vec3_dist(tree->t.points[0], cam_pos)),
		fmin(vec3_dist(tree->t.points[1], cam_pos), vec3_dist(tree->t.points[2], cam_pos)));
	//tree->dist = fmax(1.0, vec3_dist(tree->t.pos, cam_pos) - (TRI_BASE_LEN/sqrt(3)));
	if (depth > subdivisions_per_distance(tree->dist))
		return; //Node is divided enough, done.

	//Create children and subdivide if they don't yet exist.
	if (tree->children[0] == NULL) {
		struct terrain *new_t[NUM_TRI_DIVS];
		for (int i = 0; i < NUM_CHILDREN; i++) {
			tree->children[i] = new_tree(new_triangular_terrain(NUM_TRI_ROWS));
			new_t[i] = &(tree->children[i]->t);
		}

		subdiv_triangle_terrain(&(tree->t), new_t);
	}

	//Visit children
	for (int i = 0; i < NUM_CHILDREN; i++)
		subdivide_tree(tree->children[i], cam_pos, depth + 1);
}

void create_drawlist(PDTNODE tree, DRAWLIST *drawlist, int depth)
{
	if (depth > subdivisions_per_distance(tree->dist)) {
		if (!tree->t.buffered)
			buffer_terrain(&tree->t);
		*drawlist = drawlist_prepend(*drawlist, &tree->t);
		return;
	}

	assert(tree->children[0]); //The tree should have children if subdivide was called earlier.
	for (int i = 0; i < NUM_CHILDREN; i++)
		create_drawlist(tree->children[i], drawlist, depth + 1);
}

void prune_tree(PDTNODE tree, int depth)
{
	if (tree && depth > subdivisions_per_distance(tree->dist) + 5) {
		for (int i = 0; i < NUM_CHILDREN; i++) {
			prune_tree(tree->children[i], depth + 1);
			free_terrain(&tree->children[i]->t);
			free(tree->children[i]);
		}
		tree->children[0] = NULL;
	}
}

void free_tree(PDTNODE tree)
{
	if (tree) {
		if (tree->children[0]) {
			for (int i = 0; i < NUM_CHILDREN; i++) {
				free_tree(tree->children[i]);
			}
			tree->children[0] = NULL;
		}
		free_terrain(&tree->t);
		free(tree);
	}
}

