#include <GL/glew.h>
#include "buffer_group.h"
#include "shader_utils.h"
#include "macros.h"

void setup_attrib_for_draw(GLuint attr_handle, GLuint buffer, GLenum attr_type, int attr_size)
{
	glEnableVertexAttribArray(attr_handle);
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glVertexAttribPointer(attr_handle, attr_size, attr_type, GL_FALSE, 0, NULL);
}

struct buffer_group new_buffer_group(int (*buffering_function)(struct buffer_group), struct shader_prog *program)
{
	struct buffer_group tmp;
	glGenVertexArrays(1, &tmp.vao);
	glBindVertexArray(tmp.vao);
	glGenBuffers(LENGTH(program->attr), tmp.buffer_handles);
	for (int i = 0; i < LENGTH(program->attr); i++) {
		setup_attrib_for_draw(program->attr[i], tmp.buffer_handles[i], GL_FLOAT, 3);
	}
	glGenBuffers(1, &tmp.ibo);
	glGenBuffers(1, &tmp.aibo);
	tmp.index_count = buffering_function(tmp);
	tmp.primitive_type = GL_TRIANGLES;
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, tmp.ibo);
	return tmp;
}

struct buffer_group new_custom_buffer_group(int (*buffering_function)(struct buffer_group), int buffer_flags)
{
	struct buffer_group tmp;
	glGenBuffers(1, &tmp.vbo);
	glGenBuffers(1, &tmp.ibo);
	if (buffer_flags & BG_BUFFER_NORMALS) {
		glGenBuffers(1, &tmp.nbo);
	}
	if (buffer_flags & BG_BUFFER_COLORS) {
		glGenBuffers(1, &tmp.cbo);
	}
	tmp.index_count = buffering_function(tmp);
	tmp.primitive_type = GL_TRIANGLES;
	tmp.flags = buffer_flags;
	return tmp;
}

void delete_buffer_group(struct buffer_group tmp)
{
	glDeleteBuffers(1, &tmp.vbo);
	glDeleteBuffers(1, &tmp.ibo);
	if (tmp.flags & BG_BUFFER_NORMALS)
		glDeleteBuffers(1, &tmp.nbo);
	if (tmp.flags & BG_BUFFER_COLORS)
		glDeleteBuffers(1, &tmp.cbo);
}