#include "glsw_shaders.h"
#include "glsw/glsw.h"
#include "shader_utils.h"
#include "macros.h"
#include <stdio.h>
#include <GL/glew.h>

enum {MAX_NUM_KEYS = 5};
struct glsw_shaders glsw = {0};

GLuint shader_from_text(GLenum type, const char **texts)
{
	int num_texts;
	for (int i = 0; texts[i] != NULL; num_texts = ++i);

	//Create and compile the shader.
	GLint success = GL_FALSE;
	const GLuint shader = glCreateShader(type);

	glShaderSource(shader, num_texts, texts, NULL);
	glCompileShader(shader);
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

	//Print log if there are errors.
	if (success != GL_TRUE) {
		printf("Unable to compile!\n");
		printLog(shader, GL_FALSE);
		glDeleteShader(shader);
		//BUG(Gavin): This returns a negative value, but GLuint is unsigned...
		return -1; //Means compilation failure.
	}
	return shader;
}

GLuint glsw_shader_from_keys(GLenum type, const char **keys)
{
	int num_texts;
	for (int i = 0; keys[i] != NULL; num_texts = ++i);

	const char *texts[num_texts + 1];
	for (int i = 0; i < num_texts; i++)
		texts[i] = glswGetShader(keys[i]);
	texts[num_texts] = NULL;
	return shader_from_text(type, texts);
}

void glsw_shaders_init()
{
	glswInit();
	glswSetPath("shaders/glsw/", ".glsl");
	glswAddDirectiveToken("GL33", "#version 330");

	//TODO(Gavin): Have some form of error handling.

	GLuint forward_shader[] = {
		glsw_shader_from_keys(GL_VERTEX_SHADER,   (const char *[]){"forward.vertex.GL33", NULL}),
		glsw_shader_from_keys(GL_FRAGMENT_SHADER, (const char *[]){"forward.fragment.GL33", "common.lighting", NULL}),
	};

	GLuint forward_program = glCreateProgram();
	if (compile_shader_program(forward_program, forward_shader, LENGTH(forward_shader)) == 0) {
		glDeleteProgram(glsw.forward);
		glsw.forward = forward_program;
	} else {
		glDeleteProgram(forward_program);
	}

	for (int i = 0; i < LENGTH(forward_shader); i++)
		glDeleteShader(forward_shader[i]);

	glswShutdown();
}