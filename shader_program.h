#ifndef SHADER_PROGRAM_H
#define SHADER_PROGRAM_H
#include <GL/glew.h>

struct shader_prog {
	const GLchar **vs_source;
	const GLchar **fs_source;
	GLuint handle;
	int attr_cnt;
	GLint *attr;
	const GLchar **attr_names;
};

#endif