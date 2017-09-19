#ifndef MESH_H
#define MESH_H
#include <GL/glew.h>
#include "glla.h"

/*
A mesh consists of vertices, edges and faces (implicit or explicit), defining a 3D surface.
Vertices may have associated normals, colors, and, texture coordinates, with perhaps other properties as necessary.
The index buffer defining faces from vertices is also stored in the mesh, as well as an index buffer with adjacency information, if necessary.

Meshes with the same vertex format can share a vertex array object and even store their vertices in the same buffer object, so offsets in such
a buffer are stored as well.
*/

//Type 0: A generic mesh, with enough information to define a colored, shaded object.
struct geo_mesh_t0 {
	struct {
		vec3 position, normal;
		unsigned char color[3];
	} *vertices;
	unsigned int num_vertices;
	unsigned int *indices;
	unsigned int *indices_adjacency;
	unsigned int num_indices;
	unsigned int num_indices_adjacency;
	struct {
		size_t positions, normals, colors;
	} buffer_object_offsets;
};

extern struct geo_mesh_t0_OpenGL_buffer_handles {
	GLuint vertex_array_object, indices, indices_adjacency, positions, normals, colors;
} geo_mesh_t0_OpenGL;

#endif