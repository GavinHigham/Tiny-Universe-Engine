#ifndef GLSW_SHADERS_H
#define GLSW_SHADERS_H
#include <GL/glew.h>

struct glsw_shaders {
	GLuint forward;
} glsw;

void glsw_shaders_init();

#endif