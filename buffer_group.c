#include <GL/glew.h>
#include "buffer_group.h"

struct buffer_group new_buffer_group(int (*buffering_function)(struct buffer_group))
{
	struct buffer_group tmp;
	glGenBuffers(1, &tmp.vbo);
	glGenBuffers(1, &tmp.nbo);
	glGenBuffers(1, &tmp.cbo);
	glGenBuffers(1, &tmp.ibo);
	tmp.index_count = buffering_function(tmp);
	return tmp;
}

struct buffer_group new_custom_buffer_group(int (*buffering_function)(struct buffer_group), int buffer_flags)
{
	struct buffer_group tmp;
	glGenBuffers(1, &tmp.vbo);
	glGenBuffers(1, &tmp.ibo);
	if (buffer_flags & BG_BUFFER_NORMALS)
		glGenBuffers(1, &tmp.nbo);
	if (buffer_flags & BG_BUFFER_COLORS)
		glGenBuffers(1, &tmp.cbo);
	tmp.index_count = buffering_function(tmp);
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