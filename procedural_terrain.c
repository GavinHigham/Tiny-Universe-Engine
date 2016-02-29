#include <stdlib.h>
#include <math.h>
#include <GL/glew.h>
#include "procedural_terrain.h"
#include "math/vector3.h"
#include "buffer_group.h"
#include "macros.h"
#include "render.h"

extern int PRIMITIVE_RESTART_INDEX;

VEC3 height_map1(float x, float z)
{
	float height = 0;
	int octaves = 5;
	for (int i = 1; i <= octaves; i++)
		height += i*sin(z/i) + i*sin(x/i);
	return (VEC3){{x, height, z}};
}

//Cheap trick to get normals, should replace with something faster eventually.
VEC3 height_map_normal1(float x, float z)
{
	float delta = 0.0001;
	VEC3 v0 = height_map1(x, z);
	VEC3 v1 = height_map1(x+delta, z);
	VEC3 v2 = height_map1(x, z+delta);

	return vec3_normalize(vec3_cross(vec3_sub(v2, v0), vec3_sub(v1, v0)));
	//return (VEC3){{0, 1, 0}};
}

struct buffer_group buffer_grid(int numrows, int numcols)
{
	//Sort of green color
	VEC3 color = {{0.8, 0.8, 0.8}};
	struct buffer_group tmp;
	tmp.index_count = (2 * numcols + 1) * (numrows - 1);
	glGenVertexArrays(1, &tmp.vao);
	glBindVertexArray(tmp.vao);
	glGenBuffers(1, &tmp.ibo);
	glGenBuffers(LENGTH(tmp.buffer_handles), tmp.buffer_handles);
	int atrlen = sizeof(VEC3) * numrows * numcols;
	int indlen = sizeof(GLuint) * tmp.index_count;
	VEC3 *positions = (VEC3 *)malloc(atrlen);
	VEC3 *normals = (VEC3 *)malloc(atrlen);
	VEC3 *colors = (VEC3 *)malloc(atrlen);
	GLuint *indices = (GLuint *)malloc(indlen);

	//Generate vertices.
	for (int i = 0; i < numrows; i++) {
		for (int j = 0; j < numcols; j++) {
			int offset = (numcols * i) + j;
			VEC3 pos = height_map1(i, j);
			VEC3 norm = height_map_normal1(i, j);
			positions[offset] = pos;
			normals[offset] = norm;
			colors[offset] = color;
		}
	}
	//Generate indices
	for (int i = 0; i < (numrows - 1); i++) {
		int j = 0;
		int col_offset = i*(numcols*2+1);
		for (j = 0; j < numcols; j++) {
			indices[col_offset + j*2]     = i*numcols + j + numcols;
			indices[col_offset + j*2 + 1] = i*numcols + j;
		}
		indices[col_offset + j*2] = PRIMITIVE_RESTART_INDEX;
	}

	setup_attrib_for_draw(forward_program.vPos,    tmp.vbo, GL_FLOAT, 3);
	setup_attrib_for_draw(forward_program.vNormal, tmp.nbo, GL_FLOAT, 3);
	setup_attrib_for_draw(forward_program.vColor,  tmp.cbo, GL_FLOAT, 3);
	glEnable(GL_PRIMITIVE_RESTART);
	glPrimitiveRestartIndex(PRIMITIVE_RESTART_INDEX);
	tmp.primitive_type = GL_TRIANGLE_STRIP;
	glBindBuffer(GL_ARRAY_BUFFER, tmp.vbo);
	glBufferData(GL_ARRAY_BUFFER, atrlen, positions, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, tmp.nbo);
	glBufferData(GL_ARRAY_BUFFER, atrlen, normals, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, tmp.cbo);
	glBufferData(GL_ARRAY_BUFFER, atrlen, colors, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, tmp.ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indlen, indices, GL_STATIC_DRAW);

	free(positions);
	free(normals);
	free(colors);
	free(indices);

	return tmp;
}