#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "quadtree.h"

enum { NCHILDREN = QUADTREE_NUM_CHILDREN };

static bool node_has_children(quadtree_node *tree)
{
	return tree->children[0] != NULL;
}

static void node_set_childless(quadtree_node *tree)
{
	tree->children[0] = NULL;
}

quadtree_node * quadtree_new(void *data, int depth)
{
	quadtree_node *new = malloc(sizeof(struct quadtree_node));
	//Fix notes: before, each tree node contained the struct directly. Now that it's a pointer, I need to allocate tiles dynamically.
	new->data = data;
	new->depth = depth;
	node_set_childless(new);
	return new;
}

void quadtree_node_add_children(quadtree_node *node, void *child_data[NCHILDREN])
{
	for (int i = 0; i < NCHILDREN; i++)
		node->children[i] = quadtree_new(child_data[i], node->depth + 1);
}

void quadtree_preorder_visit(quadtree_node *tree, quadtree_visit_fn visit, void *context)
{
	if (!tree || !visit(tree, context) || !node_has_children(tree))
		return; //Node is divided enough, done.

	//Visit children
	for (int i = 0; i < NCHILDREN; i++)
		quadtree_preorder_visit(tree->children[i], visit, context);
}

void quadtree_postorder_visit(quadtree_node *tree, quadtree_visit_fn visit, void *context)
{
	if (!tree)
		return;

	if (node_has_children(tree))
		for (int i = 0; i < NCHILDREN; i++)
			quadtree_postorder_visit(tree->children[i], visit, context);

	visit(tree, context);
}

bool quadtree_node_free(quadtree_node *node, void *context)
{
	if (node && context) {
		void (*free_data)(void *data) = context;
		free_data(node->data);
		free(node);
		return true;
	}
	return false;
}

void quadtree_free(quadtree_node *tree, void (*free_data)(void *data), bool children_only)
{
	if (children_only)
		return quadtree_postorder_visit(tree, quadtree_node_free, free_data);

	if (tree && node_has_children(tree))
		for (int i = 0; i < NCHILDREN; i++)
			quadtree_free(tree->children[i], free_data, false);
	node_set_childless(tree);
}
