#ifndef MESH_H
#define MESH_H
#include <stdbool.h>
#include "glla.h"
#include "graphics.h"
#include "effects.h"

/*
A mesh consists of vertices, edges and faces (implicit or explicit), defining a 3D surface.
Vertices may have associated normals, colors, and, texture coordinates, with perhaps other properties as necessary.
The index buffer defining faces from vertices is also stored in the mesh, as well as an index buffer with adjacency information, if necessary.

Meshes with the same vertex format can share a vertex array object and even store their vertices in the same buffer object, so offsets in such
a buffer are stored as well.
*/

enum geo_mesh_vertex_type {
	GEO_MESH_VERTEX_POS_NORM_COL,
	//...
	GEO_MESH_NUM_VERTEX_TYPES,
};

//A vertex for a colored, shaded, un-textured mesh.
struct vertex_pos_norm_col {
	vec3 position, normal;
	unsigned char color[3];
};
//...

struct geo_mesh {
	enum geo_mesh_vertex_type type; //Corresponds to the vertex type.
	unsigned int num_vertices;
	unsigned int **indices; //Double pointer because the indices may be reallocated.
	unsigned int **indices_adjacency; //Indices including adjacency, for stencil shadows, outlines, etc.
	unsigned int num_indices;
	unsigned int num_indices_adjacency; //num_indices * 2

	GLint base_vertex; //Offset within the vbo of its type.
	GLenum mode; //Draw mode: GL_TRIANGLES, GL_TRIANGLE_FAN, etc.

	union {
		struct vertex_pos_norm_col **vertices_pnc; //Double pointer because vertices may be reallocated.
		//...
		void *vertices_none;
	};
};

extern struct geo_mesh_OpenGL_buffer_handles {
	GLuint vertex_array_object, indices, indices_adjacency, vertices;
	bool is_init;
} geo_mesh_OpenGL;

struct geo_mesh * geo_mesh_new(enum geo_mesh_vertex_type type, unsigned int num_vertices, unsigned int num_indices);
void geo_mesh_vertex_type_init(enum geo_mesh_vertex_type type, EFFECT effect);

#endif