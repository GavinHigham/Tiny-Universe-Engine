#ifndef BUFFER_GROUP_H
#define BUFFER_GROUP_H
#include <GL/glew.h>
#include "shaders/shaders.h"

enum {
	BG_BUFFER_NORMALS    = 1,
	BG_BUFFER_COLORS     = 2,
	BG_USING_ADJACENCIES = 4 //These are bitflags, each new flag should be double the last one.
};

//Add more vertex attribute buffer flags as powers-of-two

struct buffer_group {
	GLuint vao, ibo, aibo;
	union {
		struct {
			GLuint nbo, cbo, vbo;
		};
		GLuint buffer_handles[sizeof(shader_programs[0]->attr)/sizeof(shader_programs[0]->attr[0])];
	};
	int index_count;
	int flags;
	GLenum primitive_type;
};

struct buffer_group new_buffer_group(int (*buffering_function)(struct buffer_group), struct shader_prog *program);
struct buffer_group new_custom_buffer_group(int (*buffering_function)(struct buffer_group), int buffer_flags, GLenum primitive_type);
void delete_buffer_group(struct buffer_group tmp);
void setup_attrib_for_draw(GLuint attr_handle, GLuint buffer, GLenum attr_type, int attr_size);

#endif