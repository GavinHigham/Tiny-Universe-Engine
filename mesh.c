#include <stdlib.h>
#include "mesh.h"
#include "effects.h"

/*
A VAO (among other things) tells a shader how to interpret a vertex buffer to retrieve vertex attributes.
I can set up code to automatically have a "color" attribute in a shader read from the right part of a vertex buffer.
Alternatively, I can just set the offsets from the shader, and have a simple mapping from layout to vertex attribute.
I'm already implicitly doing this using the order of vertex attributes, so I might want to just make the change.
*/
#define BUFFER_OFFSET(i) ((char *)NULL + (i))

struct geo_mesh_OpenGL_buffer_handles geo_mesh_OpenGL = {
	//GLuint indices, indices_adjacency, vertex;
	0
};

GLuint geo_mesh_OpenGL_VAOs[] = { 0 }; // Need one for each type of vertex.

struct geo_mesh * geo_mesh_new(enum geo_mesh_vertex_type type, unsigned int num_vertices, unsigned int num_indices)
{
	struct geo_mesh *mesh = malloc(sizeof(struct geo_mesh));
	mesh->type = type;
	mesh->num_vertices = num_vertices;
	mesh->num_indices = num_indices;
	mesh->num_indices_adjacency = 2 * num_indices;
	switch (type) {
	case GEO_MESH_VERTEX_POS_NORM_COL:
		mesh->vertices_pnc = malloc(sizeof(struct vertex_pos_norm_col)*num_vertices);
		break;
	default:
		mesh->vertices_none = NULL;
		break;
	}
	
	mesh->indices = malloc(sizeof(unsigned int) * num_indices);
	mesh->indices_adjacency = malloc(sizeof(unsigned int) * num_indices * 2);

	return mesh;
}

void geo_mesh_free(struct geo_mesh *mesh)
{
	//free(mesh->t0_vertices);
	//free(mesh->indices);
	//free(mesh->indices_adjacency);
	free(mesh);
}

static void init_vertex_pos_norm_col(EFFECT effect)
{
	size_t vsize = sizeof(struct vertex_pos_norm_col);
	glBindBuffer(GL_ARRAY_BUFFER, geo_mesh_OpenGL.vertices);
	glVertexAttribPointer(effect.vPos,    sizeof(GLfloat), GL_FLOAT, GL_FALSE, vsize, BUFFER_OFFSET(0));
	glVertexAttribPointer(effect.vNormal, sizeof(GLfloat), GL_FLOAT, GL_FALSE, vsize, BUFFER_OFFSET(sizeof(GLfloat)*3));
	glVertexAttribPointer(effect.vColor,  sizeof(GLchar),  GL_BYTE,  GL_TRUE,  vsize, BUFFER_OFFSET(sizeof(GLfloat)*6));
	glEnableVertexAttribArray(effect.vPos);
	glEnableVertexAttribArray(effect.vNormal);
	glEnableVertexAttribArray(effect.vColor);
}

void geo_mesh_vertex_type_init(enum geo_mesh_vertex_type type, EFFECT effect)
{
	if (!geo_mesh_OpenGL_VAOs[type]) {
		glGenVertexArrays(1, &geo_mesh_OpenGL_VAOs[type]);
		glBindVertexArray(geo_mesh_OpenGL_VAOs[type]);

		switch(type) {
		case GEO_MESH_VERTEX_POS_NORM_COL: init_vertex_pos_norm_col(effect); break;
		default: break;
		}
	}

	// t->bg.primitive_type = GL_TRIANGLE_STRIP;
	// glGenVertexArrays(1, &t->bg.vao);
	// glBindVertexArray(t->bg.vao);
	// glGenBuffers(LENGTH(t->bg.buffer_handles), t->bg.buffer_handles);
}