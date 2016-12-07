#include <glalgebra.h>
#include <math.h>
#include <assert.h>
#include "procedural_terrain.h"
#include "dynamic_terrain.h"
#include "buflist.h"
#include "render.h"

/*
A line segment of length l, perpendicular to the camera view vector at distance d, on a screen w pixels wide will be
(w * l) / (d * 2) pixels across.

A triangular tile with n rows and base length L will have triangle primitives with base length L/n (unitless).

The widest such a triangle primitive can appear on the screen if the nearest vertex of the triangular tile is distance d is
(w * L/n) / (d * 2) pixels across. The narrowest is (w * L/n) / ((d+L) * 2).

The widest such a triangle primitive can appear on the screen if the center point of the triangular tile is distance d is
(w * L/n) / ((d - L/sqrt(3)) * 2) pixels across (not defined if (d - L/sqrt(3)) < 1). The narrowest is (w * L/n) / ((d + L/sqrt(3)) * 2).

A triangular tile with s subdivisions has effectively 2^s * n rows.
Thus, to keep the pixel width p of any primitive within a certain ranges:

Using n = nearest tile vertex
	No wider than p
		s = log2(((w * L/n) / (d * 2 * p))
	No narrower than p
		s = log2((w * L/n) / ((d+L) * 2 * p))

Using n = center point of tile
	No wider than p
		s = log2((w * L/n) / ((d - L/sqrt(3)) * 2 * p))
	No narrower than p
		s = log2((w * L/n) / ((d + L/sqrt(3)) * 2 * p))
*/

int subdivisions_per_distance(float distance)
{
	float biased_d = distance + TRI_BASE_LEN;
	return log2((screen_width * TRI_BASE_LEN/NUM_TRI_ROWS) / ((biased_d * 2 * PIXELS_PER_TRI)));
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
	tree->dist = vec3_dist(tree->t.pos, cam_pos);
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

