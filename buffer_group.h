#ifndef BUFFER_GROUP_H
#define BUFFER_GROUP_H
#include <GL/glew.h>

#define BG_BUFFER_NORMALS   1
#define BG_BUFFER_COLORS    2
//Add more vertex attribute buffer flags as powers-of-two

struct buffer_group {
	GLuint vbo, cbo, nbo, ibo;
	int index_count;
	int flags;
};

struct buffer_group new_buffer_group(int (*buffering_function)(struct buffer_group));
struct buffer_group new_custom_buffer_group(int (*buffering_function)(struct buffer_group), int buffer_flags);
void delete_buffer_group(struct buffer_group tmp);

#endif