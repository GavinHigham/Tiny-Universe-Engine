#include <glla.h>
#include <math.h>
#include <assert.h>
#include <stdio.h>
#include "dynamic_terrain.h"
#include "triangular_terrain_tile.h"
#include "buflist.h"
#include "render.h"
#include "terrain_types.h"

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


TODO:

Look at all these equations and re-evaluate them, considering that some values change with each subdivision.
Using TRI_BASE_LEN is not correct anymore.

Finding the distance to the horizon and comparing with tile centers will not always be correct.
*/

//#define SUBDIVS (dot_div_depth(tree->t.pos, tree->t.s_origin, cam_pos))
#define SUBDIVS (subdivision_depth(tree, cam_pos, planet))

int subdivisions_per_distance(float distance, float tri_pixel_width, float tile_base_len)
{
	return fmin(log2((screen_width * tile_base_len) / (distance * 2 * tri_pixel_width * DEFAULT_NUM_TRI_TILE_ROWS)), MAX_SUBDIVISIONS);
}

int dot_div_depth(vec3 tile_pos, vec3 planet_center, vec3 cam_pos)
{
	float d = fmax(0, pow(vec3_dot(vec3_normalize(vec3_sub(tile_pos, planet_center)), vec3_normalize(vec3_sub(cam_pos, planet_center))), 16));
	return pow(MAX_SUBDIVISIONS, d);
}

int subdivision_depth(PDTNODE tree, vec3 cam_pos, proc_planet *planet)
{
	float h = vec3_dist(cam_pos, planet->pos) - planet->radius; //If negative, we're below sea level.
	h = fmax(h, 0); //Don't handle "within the planet" case yet.
	float d = sqrt(2*planet->radius*h + h*h);
	if (tree->dist > d + planet->edge_len/pow(2.0, tree->depth + 1)) //Node is beyond the horizon.
		return 0;
	else
		return subdivisions_per_distance(h, 40, planet->edge_len);
}

DRAWLIST drawlist_prepend(DRAWLIST list, tri_tile *t)
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

bool tree_has_children(PDTNODE tree)
{
	return tree->children[0] != NULL;
}

void tree_set_childless(PDTNODE tree)
{
	tree->children[0] = NULL;
}

PDTNODE new_tree(tri_tile t, int depth)
{
	PDTNODE new = malloc(sizeof(struct dynamic_terrain_node));
	new->t = t;
	new->depth = depth;
	tree_set_childless(new);
	return new;
}

void subdivide_tree(PDTNODE tree, vec3 cam_pos, proc_planet *planet)
{
	assert(tree);
	
	// tree->dist = fmin(
	// 	vec3_dist(tree->t.points[0], cam_pos),
	// 	fmin(
	// 		vec3_dist(tree->t.points[1], cam_pos),
	// 		vec3_dist(tree->t.points[2], cam_pos)));
	tree->dist = vec3_dist(tree->t.pos, cam_pos);

	if (tree->depth > SUBDIVS)
		return; //Node is divided enough, done.

	//Cap the number of subdivisions per whole-tree traversal (per frame essentially)
	static int remaining_subdivisions = 0;
	if (tree->depth == 0)
		remaining_subdivisions = YIELD_AFTER_DIVS;
	if (remaining_subdivisions <= 0)
	 	return;

	//Create children and subdivide if they don't yet exist.
	if (!tree_has_children(tree)) {
		printf("Subdiving node at <%f, %f, %f>, depth %i\n", tree->t.pos.x, tree->t.pos.y, tree->t.pos.z, tree->depth);
		tri_tile *new_t[DEFAULT_NUM_TRI_TILE_DIVS];
		for (int i = 0; i < DEFAULT_NUM_TRI_TILE_DIVS; i++) {
			tree->children[i] = new_tree((tri_tile){.is_init = false}, tree->depth + 1);
			new_t[i] = &(tree->children[i]->t);
		}

		remaining_subdivisions--;
		subdiv_tri_tile(&(tree->t), new_t);
	}

	//Visit children
	for (int i = 0; i < DEFAULT_NUM_TRI_TILE_DIVS; i++)
		subdivide_tree(tree->children[i], cam_pos, planet);
}

void create_drawlist(PDTNODE tree, DRAWLIST *drawlist, vec3 cam_pos, proc_planet *planet)
{
	if (tree->depth > SUBDIVS || !tree_has_children(tree)) {
		if (!tree->t.buffered)
			buffer_tri_tile(&tree->t);
		*drawlist = drawlist_prepend(*drawlist, &tree->t);
		return;
	}

	//assert(tree->children[0]); //The tree should have children if subdivide was called earlier.
	for (int i = 0; i < DEFAULT_NUM_TRI_TILE_DIVS; i++)
		create_drawlist(tree->children[i], drawlist, cam_pos, planet);
}

void prune_tree(PDTNODE tree, vec3 cam_pos, proc_planet *planet)
{
	if (tree && tree->depth > SUBDIVS + 5) {
		if (tree_has_children(tree)) {
			for (int i = 0; i < DEFAULT_NUM_TRI_TILE_DIVS; i++) {
				prune_tree(tree->children[i], cam_pos, planet);
				deinit_tri_tile(&tree->children[i]->t);
				free(tree->children[i]);
			}
			tree_set_childless(tree);
		}
	}
}

void free_tree(PDTNODE tree)
{
	if (tree) {
		if (tree->children[0]) {
			for (int i = 0; i < DEFAULT_NUM_TRI_TILE_DIVS; i++) {
				free_tree(tree->children[i]);
			}
			tree_set_childless(tree);
		}
		deinit_tri_tile(&tree->t);
		free(tree);
	}
}

