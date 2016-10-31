#include <glalgebra.h>
#include "dynamic_terrain.h"
#include "buflist.h"

//Might want to have values for "minimum pixels per triangle" and "maximum pixels per triangle"
//For that I need an equation that relates the number of pixels a triangle occupies with its size and distance.
//Diameter scales linearly with distance
//Triangle size can just be size of a tile / #triangles per row of tile

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

//Comparison function to order tiles by precomputed distance.
int dt_node_distance_compare(const void *n1, const void *n2)
{
	return ((PDTNODE)n1)->dist > ((PDTNODE)n2)->dist;
}

// int dt_node_closeness_compare(const void *n1, const void *n2)
// {
// 	return ((PDTNODE)n1)->dist < ((PDTNODE)n2)->dist;
// }

//Create higher-detail tiles to replace a lower-detail one.
static void dt_subdivide(PDTNODE root, STACK *pool)
{
	for (int i = 0; i < NUM_CHILDREN; i++) {
		root->children[i] = stack_pop(pool);
		//children positioning happens here
	}
}

//Traverses an entire subtree, recycling every node.
static void dt_recycle_subtree_children(PDTNODE root, STACK *pool) {
	if (root == NULL || root->children[0] == NULL)
		return;
	for (int i = 0; i < NUM_CHILDREN; i++) {
		dt_recycle_subtree_children(root->children[i], pool);
		stack_push(pool, root->children[i]);
		root->children[i] = NULL;
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

//Subdivides a terrain tile based on distance to camera,
//and puts any sufficiently-divided tile into the drawlist.
//Recycles most distant tiles into the pool if needed.
int dt_add_children(PDTNODE current, STACK *pool, HEAP *drawlist, vec3 cam_pos, int splits)
{
	bool success = true;
	current->dist = vec3_dist(current->midpoint, cam_pos);
	//Find out how many times a tile should be split. Closer means more splits.
	int total_splits = dt_depth_per_distance(current->dist);
	//If the node we're visiting has been split enough, we're done.
	if (splits == total_splits) {
		heap_add(drawlist, current, dt_node_distance_compare);
		return success;
	}

	//Check if there are enough free nodes in the pool for new children.
	int available = stack_available(pool);
	if (NUM_CHILDREN > available)
		dt_recycle_distant_tiles(pool, drawlist, NUM_CHILDREN - available);

	//Generate new child terrain tiles.
	dt_subdivide(current, pool);
		
	//Visit the new children to see if they need to be subdivided further.
	for (int i = 0; i < NUM_CHILDREN; i++)
		dt_add_children(current->children[i], pool, drawlist, cam_pos, splits+1);

	return success;
}