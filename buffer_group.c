#include <GL/glew.h>
#include <stdio.h>
#include <string.h>
#include "buffer_group.h"
#include "shader_utils.h"
#include "math/utility.h"
#include "macros.h"

void setup_attrib_for_draw(GLuint attr_handle, GLuint buffer, GLenum attr_type, int attr_size)
{
	glEnableVertexAttribArray(attr_handle);
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	//TODO: Use normalized = TRUE and an array of bytes for color information.
	glVertexAttribPointer(attr_handle, attr_size, attr_type, GL_FALSE, 0, NULL);
}

int buffer_group_attribute_index(const char *attr_name)
{
	for (int i = 0; i < LENGTH(attribute_strings); i++)
		if (!strcmp(attr_name, attribute_strings[i]))
			return i;
	return -1;
}

struct buffer_group new_buffer_group(int (*buffering_function)(struct buffer_group), EFFECT *effect)
{
	struct buffer_group tmp;
	glGenVertexArrays(1, &tmp.vao);
	glBindVertexArray(tmp.vao);
	glGenBuffers(1, &tmp.aibo);
	glGenBuffers(1, &tmp.ibo);

	for (int i = 0; i < LENGTH(effect->attr); i++) {
		if (effect->attr[i] != -1) {
			glGenBuffers(1, &tmp.buffer_handles[i]);
			setup_attrib_for_draw(effect->attr[i], tmp.buffer_handles[i], GL_FLOAT, 3);
		}
	}

	tmp.primitive_type = GL_TRIANGLES;
	tmp.index_count = buffering_function(tmp);
	tmp.flags = BG_USING_ADJACENCIES | BG_BUFFER_NORMALS | BG_BUFFER_COLORS;
	//glBindVertexArray(0);
	return tmp;
}

struct buffer_group new_custom_buffer_group(int (*buffering_function)(struct buffer_group), int buffer_flags, GLenum primitive_type)
{
	struct buffer_group tmp = {0};
	glGenVertexArrays(1, &tmp.vao);
	glBindVertexArray(tmp.vao);
	glGenBuffers(1, &tmp.vbo);
	glGenBuffers(1, &tmp.ibo);

	if (buffer_flags & BG_USING_ADJACENCIES)
		glGenBuffers(1, &tmp.aibo);
	if (buffer_flags & BG_BUFFER_NORMALS)
		glGenBuffers(1, &tmp.nbo);
	if (buffer_flags & BG_BUFFER_COLORS)
		glGenBuffers(1, &tmp.cbo);

	tmp.flags = buffer_flags;
	tmp.primitive_type = primitive_type;
	tmp.index_count = buffering_function(tmp);
	//glBindVertexArray(0);
	return tmp;
}

void delete_buffer_group(struct buffer_group tmp)
{
	glDeleteBuffers(1, &tmp.vbo);
	glDeleteBuffers(1, &tmp.ibo);
	//These buffers are optional, but OpenGL ignores the calls if the handles are 0 or invalid.
	glDeleteBuffers(1, &tmp.aibo);
	glDeleteBuffers(1, &tmp.nbo);
	glDeleteBuffers(1, &tmp.cbo);

	glDeleteVertexArrays(1, &tmp.vao);
}