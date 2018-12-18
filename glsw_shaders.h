#ifndef GLSW_SHADERS_H
#define GLSW_SHADERS_H
#include "graphics.h"

struct glsw_shaders {
	GLuint forward;
} glsw;

void glsw_shaders_init();
GLuint glsw_new_shader_program(GLuint shaders[], int num);
GLuint glsw_shader_from_keys_num(GLenum type, const char **keys, int num);
#define glsw_shader_from_keys(type, ...) glsw_shader_from_keys_num(type, (const char *[]){__VA_ARGS__}, sizeof((const char *[]){__VA_ARGS__})/sizeof(char *))

#endif