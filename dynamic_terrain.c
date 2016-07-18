#include <glalgebra.h>
#include "dynamic_terrain.h"
#include "buflist.h"

int dt_depth_per_distance(float distance)
{
	if (distance > 1000)
		return 0;
	else if (distance > 700)
		return 1;
	else if (distance > 50)
		return 2;
	else
		return 3;
	//Test values.
}

int dt_node_distance_compare(const void *n1, const void *n2)
{
	return ((PDTNODE)n1)->dist > ((PDTNODE)n2)->dist;
}

// int dt_node_closeness_compare(const void *n1, const void *n2)
// {
// 	return ((PDTNODE)n1)->dist < ((PDTNODE)n2)->dist;
// }

static void dt_subdivide(PDTNODE root, STACK *pool)
{
	for (int i = 0; i < NUM_CHILDREN; i++)
		root->children[i] = stack_pop(pool);
}

static void dt_recycle_subtree_children(PDTNODE root, STACK *pool) {
	if (root == NULL || root->children[0] == NULL)
		return;
	for (int i = 0; i < NUM_CHILDREN; i++) {
		dt_recycle_subtree_children(root->children[i], pool);
		stack_push(pool, root->children[i]);
	}
}

//Recycle at least num tile nodes, taken from the furthest tiles in drawlist, into the tile pool.
static void dt_recycle_distant_tiles(STACK *pool, HEAP *drawlist, int num)
{
	PDTNODE n = heap_rem(drawlist, dt_node_distance_compare);
	dt_recycle_subtree_children(n, pool); //Recycle the subchildren of the furthest tile.
	int available = stack_available(pool);
	if (num > available) //If we need more space,
		dt_recycle_distant_tiles(pool, drawlist, num - available); //continue with the next furthest.
	heap_add(drawlist, n, dt_node_distance_compare);
}

int dt_add_children(PDTNODE root, STACK *pool, HEAP *drawlist, vec3 cam_pos, int depth)
{
	bool success = true;
	root->dist = vec3_dist(root->frame.t, cam_pos);
	int correct_depth = dt_depth_per_distance(root->dist);
	if (depth == correct_depth) {
		heap_add(drawlist, root, dt_node_distance_compare);
		return success;
	}

	int available = stack_available(pool);
	if (NUM_CHILDREN > available)
		dt_recycle_distant_tiles(pool, drawlist, NUM_CHILDREN - available);
	dt_subdivide(root, pool);
		
	for (int i = 0; i < NUM_CHILDREN; i++)
		dt_add_children(root->children[i], pool, drawlist, cam_pos, depth+1);

	return success;
}