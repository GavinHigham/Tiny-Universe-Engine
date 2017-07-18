/*

- Stars Requirements -

Stars are points visible in space around the camera to some large radius R. Stars must continuously be generated outside 
 that radius. There should be a way to query for stars that are nearby. It should be simple to pick any point in space, 
and have the correct stars generated and stored in a deterministic way.

- Stars Implementation -
Stars are stored in "star blocks", cubes with edge length equal to the maximum visible distance of a star. There should 
be 27 such blocks, packed into in a 3x3x3 grid. At the center of the grid is the block the camera is currently inside. 
Every frame, all 27 blocks are visited, determining the following:
	Which block is the camera currently in?
	Do any blocks need to be updated?

Stars should already be generated for 1 R before and ahead of the camera on each axis. If a block's center is more than 
1 R (manhattan distance) from the center of the camera's current block, that block should be updated.

*/

void stars_init();
void stars_deinit();
void stars_draw();