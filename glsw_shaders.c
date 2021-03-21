#include "glsw_shaders.h"
#include "glsw/glsw.h"
#include "shader_utils.h"
#include "macros.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <GL/glew.h>

enum {MAX_NUM_KEYS = 5};
struct glsw_shaders glsw = {0};

GLuint shader_from_strs(GLenum type, const char **strs, int count)
{
	//Create and compile the shader.
	GLint success = GL_FALSE;
	const GLuint shader = glCreateShader(type);

	glShaderSource(shader, count, strs, NULL);
	glCompileShader(shader);
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

	//Print log if there are errors.
	if (success != GL_TRUE) {
		printf("Unable to compile!\n");
		printLog(shader, GL_FALSE);
		glDeleteShader(shader);
		//BUG(Gavin): This returns a negative value, but GLuint is unsigned...
		return 0; //Means compilation failure.
	}
	return shader;
}

GLuint glsw_shader_from_keys_num(GLenum type, const char **keys, int num)
{
	const char *strs[num];
	for (int i = 0; i < num; i++) {
		assert(strcmp(keys[i], "")); //Ensure we don't get an empty string key.
		strs[i] = glswGetShader(keys[i]);
		if (!strs[i]) {
			printf("Could not find shader with key %s\n", keys[i]);
			return 0;
		}
	}
	return shader_from_strs(type, strs, num);
}

GLuint glsw_new_shader_program(GLuint shaders[], int num)
{
	GLuint program = glCreateProgram();
	if (!compile_shader_program(program, shaders, num)) {
		glDeleteProgram(program);
		program = 0;
	}

	for (int i = 0; i < num; i++)
		glDeleteShader(shaders[i]);

	return program;
}

void glsw_shaders_init()
{
	glswInit();
	glswSetPath("shaders/glsw/", ".glsl");
	glswAddDirectiveToken("GL33", "#version 330");

	//TODO(Gavin): Have some form of error handling.

	GLuint forward_shader[] = {
		glsw_shader_from_keys(GL_VERTEX_SHADER, "forward.vertex.GL33"),
		glsw_shader_from_keys(GL_FRAGMENT_SHADER, "forward.fragment.GL33", "common.lighting"),
	};
	GLuint forward_program = glsw_new_shader_program(forward_shader, LENGTH(forward_shader));
	if (forward_program) {
		glDeleteProgram(glsw.forward);
		glsw.forward = forward_program;
	}

	glswShutdown();
}