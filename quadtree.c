#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "quadtree.h"

//Just make this a little shorter for code clarity.
#define NCHILDREN QUADTREE_NUM_CHILDREN

quadtree_node * quadtree_new(void *data, int depth)
{
	quadtree_node *new = malloc(sizeof(struct quadtree_node));
	//Fix notes: before, each tree node contained the struct directly. Now that it's a pointer, I need to allocate tiles dynamically.
	new->data = data;
	new->depth = depth;
	quadtree_node_set_childless(new);
	return new;
}

void quadtree_node_add_children(quadtree_node *node, void *child_data[NCHILDREN])
{
	for (int i = 0; i < NCHILDREN; i++)
		node->children[i] = quadtree_new(child_data[i], node->depth + 1);
}

void quadtree_preorder_visit(quadtree_node *tree, quadtree_visit_fn visit, void *context)
{
	if (!tree || !visit(tree, context) || !quadtree_node_has_children(tree))
		return; //Node is divided enough, done.

	//Visit children
	for (int i = 0; i < NCHILDREN; i++)
		quadtree_preorder_visit(tree->children[i], visit, context);
}

void quadtree_postorder_visit(quadtree_node *tree, quadtree_visit_fn visit, void *context)
{
	if (!tree)
		return;

	if (quadtree_node_has_children(tree))
		for (int i = 0; i < NCHILDREN; i++)
			quadtree_postorder_visit(tree->children[i], visit, context);

	visit(tree, context);
}

bool quadtree_node_free(quadtree_node *node, void *free_data)
{
	if (node && free_data) {
		//Cast free_data to a function taking a void * and returning void, and call it.
		((void (*)(void *))free_data)(node->data);
		free(node);
		return true;
	}
	return false;
}

void quadtree_free(quadtree_node *tree, void (*free_data)(void *data))
{
	quadtree_postorder_visit(tree, quadtree_node_free, free_data);
}

bool quadtree_node_has_children(quadtree_node *tree)
{
	return tree->children[0] != NULL;
}

void quadtree_node_set_childless(quadtree_node *tree)
{
	tree->children[0] = NULL;
}
