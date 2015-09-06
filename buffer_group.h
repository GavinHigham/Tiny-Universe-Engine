#ifndef BUFFER_GROUP_H
#define BUFFER_GROUP_H
#include <GL/glew.h>

struct buffer_group {
	GLuint vbo, cbo, nbo, ibo;
	int index_count;
};

struct buffer_group new_buffer_group(int (*buffering_function)(struct buffer_group));
void delete_buffer_group(struct buffer_group tmp);

#endif