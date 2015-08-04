#ifndef SHADER_PROGRAM_H
#define SHADER_PROGRAM_H
#include <GL/glew.h>

struct shader_prog {
	const GLchar **vs_source;
	const GLchar **fs_source;
	GLuint handle;
	int attr_cnt;
	int unif_cnt;
	GLint *attr;
	GLint *unif;
	const GLchar **attr_names;
	const GLchar **unif_names;
};

#endif