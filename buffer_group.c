#include <GL/glew.h>
#include "buffer_group.h"

struct buffer_group new_buffer_group(int (*buffering_function)(struct buffer_group))
{
	struct buffer_group tmp;
	glGenBuffers(1, &tmp.vbo);
	glGenBuffers(1, &tmp.cbo);
	glGenBuffers(1, &tmp.nbo);
	glGenBuffers(1, &tmp.ibo);
	tmp.index_count = buffering_function(tmp);
	return tmp;
}

void delete_buffer_group(struct buffer_group tmp)
{
	glDeleteBuffers(1, &tmp.vbo);
	glDeleteBuffers(1, &tmp.cbo);
	glDeleteBuffers(1, &tmp.nbo);
	glDeleteBuffers(1, &tmp.ibo);
}