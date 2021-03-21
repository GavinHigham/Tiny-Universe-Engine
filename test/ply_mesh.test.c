#include "models/ply_mesh.h"
#include "test/test_main.h"

int ply_mesh_load_cube()
{
	int nf = 0; //Number of failures

	struct ply_mesh *cube = ply_mesh_load("models/source_models/cube.ply", PLY_LOAD_GEN_IB | PLY_LOAD_GEN_AIB);
	TEST_SOFT_ASSERT(nf, cube);

	if (cube) {
		// printf("\n");
		ply_mesh_print(cube);
		ply_mesh_free(cube);
	}

	return nf;
}

int ply_mesh_load_newship()
{
	int nf = 0; //Number of failures

	struct ply_mesh *newship = ply_mesh_load("models/source_models/newship.ply", PLY_LOAD_GEN_IB | PLY_LOAD_GEN_AIB);
	TEST_SOFT_ASSERT(nf, newship);

	if (newship) {
		// printf("\n");
		ply_mesh_print(newship);
		ply_mesh_free(newship);
	}

	return nf;
}