#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include "vector3.h"
#include "macros.h"

float rand_float()
{
	return (float)((double)rand()/RAND_MAX); //Discard precision after the division.
}

V3 rand_box_point3d(V3 corner1, V3 corner2)
{
	V3 size = v3_sub(corner2, corner1); //size can be negative, it'll still work.
	return (V3){{{
		corner1.x + rand_float()*size.x,
		corner1.y + rand_float()*size.y,
		corner1.z + rand_float()*size.z
	}}};
}

int point_in_bounds(V3 point, V3 corner1, V3 corner2)
{
	return ( //This might not be the most efficient, but I'll review it later if it becomes a bottleneck.
		((point.x >= corner1.x && point.x <= corner2.x) || (point.x <= corner1.x && point.x >= corner2.x)) &&
		((point.y >= corner1.y && point.y <= corner2.y) || (point.y <= corner1.y && point.y >= corner2.y)) &&
		((point.z >= corner1.z && point.z <= corner2.z) || (point.z <= corner1.z && point.z >= corner2.z))
	);
}

V3 rand_sphere_shell_point3d(V3 origin, float minradius, float maxradius)
{
	float r1 = rand_float();
	float radius = r1*minradius + (1-r1)*maxradius; //Lerp between the min and max radius.
	float a1 = rand_float() * 2 * M_PI;
	float a2 = rand_float() * 2 * M_PI;
	return v3_add(origin, (V3){{{radius*cos(a1)*sin(a2), radius*sin(a1)*sin(a2), radius*cos(a2)}}});
}

struct poisson_grid {
	V3 **grid;
	V3 gd; //Grid dimensions
	V3 corner1;
	V3 corner2;
};

void poisson_grid_add_point(V3 point, struct poisson_grid *grid)
{

}

int valid_poisson_point(V3 point, struct poisson_grid *grid)
{
	V3 
	if (point_in_bounds(point, grid->corner1, grid->corner2)) {
		for (int i = -2; i < 3; i++) {
			for (int j = -2; j < 3; j++) {
				for (int k = -2; k < 3; k++) {

				}
			}
		}
	}
	return 0;
}

void poisson_point_3d(V3 *points, int numpoints, int maxpoints, struct poisson_grid *grid, V3 startpoint, float radius)
{
	//take startpoint
	//try to create 30 points around it, referencing grid
	//for each successfully created point, call poisson_point again, with that point as the new startpoint
	//stop when numpoints == maxpoints. If maxpoints == 0, keep going until no more points can be placed

	for (int i = 0; i < 30; i++) {
		V3 tmp = rand_sphere_shell_point3d(startpoint, radius, 2*radius);
		if (valid_poisson_point(tmp, grid)) {
			points[numpoints++] = tmp;
			poisson_grid_add_point(tmp, grid);
			poisson_point_3d(points, numpoints, maxpoints, grid, tmp, radius);
		}
	}
}

void poisson_points_3d(V3 *points, int maxpoints, V3 corner1, V3 corner2, float radius)
{
	float cell_size = radius/sqrt(2.0);
	struct poisson_grid grid;
	V3 grid_size = v3_sub(corner2, corner1);
	//Determine grid dimensions.
	grid.gd = v3_scale(grid_size, 1/cell_size);
	for (int i = 0; i < LENGTH(grid.gd.A); i++)
		grid.gd.A[i] = ceil(grid.gd.A[i]);

	//3d array of pointers, will access as a 1d array.
	int grid_length = grid.gd.x * grid.gd.y * grid.gd.z;
	grid.grid = malloc(sizeof(V3) * grid_length);
	for (int i = 0; i < grid_length; i++)
		grid.grid[i] = NULL;

	V3 first_point = rand_box_point3d(corner1, corner2);
	poisson_grid_add_point(first_point, &grid);
	points[0] = first_point;
	poisson_point_3d(points, 1, maxpoints, &grid, first_point, radius);

	free(grid.grid);
}

generate_poisson(width, height, min_dist, new_points_count)
{
  //Create the grid
  cellSize = min_dist/sqrt(2);

  grid = Grid2D(Point(
    (ceil(width/cell_size),         //grid width
    ceil(height/cell_size))));      //grid height

  //RandomQueue works like a queue, except that it
  //pops a random element from the queue instead of
  //the element at the head of the queue
  processList = RandomQueue();
  samplePoints = List();

  //generate the first point randomly
  //and updates 

  firstPoint = Point(rand(width), rand(height));

  //update containers
  processList.push(firstPoint);
  samplePoints.push(firstPoint);
  grid[imageToGrid(firstPoint, cellSize)] = firstPoint;

  //generate other points from points in queue.
  while (not processList.empty())
  {
    point = processList.pop();
    for (i = 0; i < new_points_count; i++)
    {
      newPoint = generateRandomPointAround(point, min_dist);
      //check that the point is in the image region
      //and no points exists in the point's neighbourhood
      if (inRectangle(newPoint) and
        not inNeighbourhood(grid, newPoint, min_dist,
          cellSize))
      {
        //update containers
        processList.push(newPoint);
        samplePoints.push(newPoint);
        grid[imageToGrid(newPoint, cellSize)] =  newPoint;
      }
    }
  }
  return samplePoints;
}