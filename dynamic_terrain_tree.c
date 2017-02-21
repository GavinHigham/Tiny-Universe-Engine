#include <glla.h>
#include <math.h>
#include <assert.h>
#include <stdio.h>
#include "renderer.h"
#include "dynamic_terrain_tree.h"
#include "dynamic_terrain_types.h"

enum { NCHILDREN = TERRAIN_TREE_NUM_CHILDREN };

/*
TODO:
Look at all these equations and re-evaluate them, considering that some values change with each subdivision.
Using TRI_BASE_LEN is not correct anymore.

Finding the distance to the horizon and comparing with tile centers will not always be correct.
*/

// Static Functions //

static bool tree_has_children(terrain_tree_node *tree)
{
	return tree->children[0] != NULL;
}

static void tree_set_childless(terrain_tree_node *tree)
{
	tree->children[0] = NULL;
}

static terrain_tree_drawlist terrain_tree_drawlist_prepend(terrain_tree_drawlist list, void *tile)
{
	struct terrain_tree_drawlist_node *new = malloc(sizeof(struct terrain_tree_drawlist_node));
	new->tile = tile;
	new->next = list;
	return new;
}

// Public functions //

terrain_tree_node * terrain_tree_new(void *tile, int depth)
{
	terrain_tree_node *new = malloc(sizeof(struct dynamic_terrain_tree_node));
	//Fix notes: before, each tree node contained the struct directly. Now that it's a pointer, I need to allocate tiles dynamically.
	new->tile = tile;
	new->depth = depth;
	tree_set_childless(new);
	return new;
}

void terrain_tree_gen(terrain_tree_node *tree, terrain_tree_depth_fn subdiv, void *context, terrain_tree_split_fn split)
{
	assert(tree);

	if (tree->depth >= subdiv(tree, context))
		return; //Node is divided enough, done.

	//Create children and subdivide if they don't yet exist.
	if (!tree_has_children(tree)) {
		//This next line should go in the "split" function when I make one.
		//printf("Subdiving node at <%f, %f, %f>, depth %i\n", tree->t.pos.x, tree->t.pos.y, tree->t.pos.z, tree->depth);
		void ** new_tiles[NCHILDREN];
		for (int i = 0; i < NCHILDREN; i++) {
			tree->children[i] = terrain_tree_new(NULL, tree->depth + 1);
			new_tiles[i] = &(tree->children[i]->tile);
		}

		split(tree->tile, new_tiles);
	}

	//Visit children
	for (int i = 0; i < NCHILDREN; i++)
		terrain_tree_gen(tree->children[i], subdiv, context, split);
}

void terrain_tree_prune(terrain_tree_node *tree, terrain_tree_depth_fn subdiv, void *context, void (*free_data)(void *))
{
	if (tree && tree->depth > subdiv(tree, context) + 5) { //subdiv function will have to return negative numbers for this to work, fix this later.
		if (tree_has_children(tree)) {
			for (int i = 0; i < NCHILDREN; i++) {
				terrain_tree_prune(tree->children[i], subdiv, context, free_data);
				free_data(&tree->children[i]->tile);
				free(tree->children[i]);
			}
			tree_set_childless(tree);
		}
	}
}

void terrain_tree_free(terrain_tree_node *tree, void (*free_data)(void *data))
{
	if (tree) {
		if (tree_has_children(tree)) {
			for (int i = 0; i < NCHILDREN; i++) {
				terrain_tree_free(tree->children[i], free_data);
			}
			tree_set_childless(tree);
		}
		free_data(tree->tile);
		free(tree);
	}
}

void terrain_tree_drawlist_new(terrain_tree_node *tree, terrain_tree_depth_fn subdiv, void *context, terrain_tree_drawlist *list)
{
	if (tree->depth >= subdiv(tree, context) || !tree_has_children(tree)) {
		*list = terrain_tree_drawlist_prepend(*list, tree->tile);
		return;
	}

	for (int i = 0; i < NCHILDREN; i++)
		terrain_tree_drawlist_new(tree->children[i], subdiv, context, list);
}

void terrain_tree_drawlist_free(terrain_tree_drawlist list)
{
	if (list) {
		terrain_tree_drawlist_free(list->next);
		free(list);
	}
}
