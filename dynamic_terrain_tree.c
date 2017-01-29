#include <glla.h>
#include <math.h>
#include <assert.h>
#include <stdio.h>
#include "buflist.h"
#include "render.h"
#include "dynamic_terrain_tree.h"

/*
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
	float d = fmax(0, pow(vec3_dot(vec3_normalize(tile_pos - planet_center), vec3_normalize(cam_pos - planet_center)), 16));
	return pow(MAX_SUBDIVISIONS, d);
}

int subdivision_depth(terrain_tree_node *tree, vec3 cam_pos, proc_planet *planet)
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

int example_subdiv(terrain_tree_node *tree, void *context)
{
	struct planet_terrain_context *ctx = (struct planet_terrain_context *)context;
	tri_tile *tile = (tri_tile *)tree->tile;
	vec3 dist = vec3_dist(tile->pos, ctx->cam_pos);

	//Cap the number of subdivisions per whole-tree traversal (per frame essentially)
	if (tree->depth == 0)
		ctx->subdivs_left = YIELD_AFTER_DIVS;
	if (ctx->subdivs_left <= 0)
	 	return 0;

	if (tree->depth >= subdivision_depth(tree, ctx->cam_pos, ctx->planet))
		return false;

	ctx->subdivs_left--;
	return true;
}

void split_tree(
	terrain_tree_node *tree,
	int (subdiv)(terrain_tree_node *, void *),
	void *context,
	int num_children,
	void (*split)(void *parent, void *children))
{
	assert(tree);

	if (tree->depth >= subdiv(tree, context))
		return; //Node is divided enough, done.

	//Create children and subdivide if they don't yet exist.
	if (!tree_has_children(tree)) {
		printf("Subdiving node at <%f, %f, %f>, depth %i\n", tree->t.pos.x, tree->t.pos.y, tree->t.pos.z, tree->depth);
		tri_tile *new_t[num_children];
		for (int i = 0; i < num_children; i++) {
			tree->children[i] = new_tree((tri_tile){.is_init = false}, tree->depth + 1);
			new_t[i] = &(tree->children[i]->t);
		}

		split(&(tree->t), new_t);
	}

	//Visit children
	for (int i = 0; i < num_children; i++)
		split_tree(tree->children[i], cam_pos, planet);
}

void prune_tree(
	terrain_tree_node *tree,
	int (subdiv)(terrain_tree_node *, void *),
	void *context,
	int num_children,
	void (*free_data)(void *data))
{
	if (tree && tree->depth > SUBDIVS + 5) {
		if (tree_has_children(tree)) {
			for (int i = 0; i < num_children; i++) {
				prune_tree(tree->children[i], cam_pos, planet);
				deinit_tri_tile(&tree->children[i]->t);
				free(tree->children[i]);
			}
			tree_set_childless(tree);
		}
	}
}

void free_tree(PDTNODE tree, void (*free_data)(void *data), int num_children)
{
	if (tree) {
		if (tree->children[0]) {
			for (int i = 0; i < num_children; i++) {
				free_tree(tree->children[i]);
			}
			tree_set_childless(tree);
		}
		free_data(&tree->t);
		free(tree);
	}
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
	for (int i = 0; i < num_children; i++)
		create_drawlist(tree->children[i], drawlist, cam_pos, planet);
}

