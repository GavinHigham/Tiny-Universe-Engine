#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "octree.h"

//Just make this a little shorter for code clarity.
#define NCHILDREN OCTREE_NUM_CHILDREN

octree_node * octree_new(void *data, int depth)
{
	octree_node *new = malloc(sizeof(struct octree_node));
	new->data = data;
	new->depth = depth;
	octree_node_set_childless(new);
	return new;
}

void octree_node_add_children(octree_node *node, void *child_data[NCHILDREN])
{
	for (int i = 0; i < NCHILDREN; i++)
		node->children[i] = octree_new(child_data[i], node->depth + 1);
}

void octree_preorder_visit(octree_node *tree, octree_visit_fn visit, void *context)
{
	if (!tree || !visit(tree, context) || !octree_node_has_children(tree))
		return; //Node is divided enough, done.

	//Visit children
	for (int i = 0; i < NCHILDREN; i++)
		octree_preorder_visit(tree->children[i], visit, context);
}

void octree_postorder_visit(octree_node *tree, octree_visit_fn visit, void *context)
{
	if (!tree)
		return;

	if (octree_node_has_children(tree))
		for (int i = 0; i < NCHILDREN; i++)
			octree_postorder_visit(tree->children[i], visit, context);

	visit(tree, context);
}

bool octree_node_free(octree_node *node, void *free_data)
{
	if (node && free_data) {
		//Cast free_data to a function taking a void * and returning void, and call it.
		((void (*)(void *))free_data)(node->data);
		free(node);
		return true;
	}
	return false;
}

void octree_free(octree_node *tree, void (*free_data)(void *data))
{
	octree_postorder_visit(tree, octree_node_free, free_data);
}

bool octree_node_has_children(octree_node *tree)
{
	return tree->children[0] != NULL;
}

void octree_node_set_childless(octree_node *tree)
{
	tree->children[0] = NULL;
}
