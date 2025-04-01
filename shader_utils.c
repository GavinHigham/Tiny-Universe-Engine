#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include "graphics.h"
#include "math/utility.h"

#ifndef LENGTH
	#define LENGTH(array) (sizeof(array)/sizeof(array[0]))
#endif


void printLog(GLuint handle, bool is_program)
{
	//Why write two log printing functions when you can use FUNCTION POINTERS AND TERNARY OPERATORS >:D
	GLboolean (*glIs)(GLuint)                                  = is_program? glIsProgram         : glIsShader;
	void (*glGetiv)(GLuint, GLenum, GLint *)                   = is_program? glGetProgramiv      : glGetShaderiv;
	void (*glGetInfoLog)(GLuint, GLsizei, GLsizei *, GLchar *) = is_program? glGetProgramInfoLog : glGetShaderInfoLog;

	if (glIs(handle)) {
		int log_length = 0, max_length = 0;
		glGetiv(handle, GL_INFO_LOG_LENGTH, &max_length);
		char *info_log = (char *)malloc(max_length);
		glGetInfoLog(handle, max_length, &log_length, info_log);
		if (log_length > 0) {
			printf("%s\n", info_log);
		}
		free(info_log);
	} else {
		printf("Name %d is not a %s.\n", handle, is_program?"program":"shader");
	}
}

const char *shader_enum_to_string(GLenum shader_type)
{
	switch (shader_type) {
	case GL_VERTEX_SHADER:   return "vertex shader";
	case GL_FRAGMENT_SHADER: return "fragment shader";
	case GL_GEOMETRY_SHADER: return "geometry shader";
	default: return "unhandled shader type";
	}
}

//Each array member of shader_texts should be free()'d if this function returns 0.
static void read_in_shaders(const char *paths[], GLchar *shader_texts[], int num)
{
	int fd, i;
	struct stat buf;
	for (i = 0; i < num; i++) {
		if (paths[i] == NULL) {
			shader_texts[i] = NULL;
			continue;
		}

		fd = open(paths[i], O_RDONLY);
		if (fd < 0) {
			printf("Could not open file: %s\n", paths[i]);
			shader_texts[i] = NULL;
			continue;
		}

		fstat(fd, &buf);
		shader_texts[i] = (GLchar *) malloc(buf.st_size+1);
		if (shader_texts[i] == NULL) {//This almost never happens.
			close(fd);
			continue;
		}

		read(fd, shader_texts[i], buf.st_size);
		close(fd);
		shader_texts[i][buf.st_size] = '\0';
	}
}

GLuint shader_new(GLenum type, const GLchar **strings, const char *path, bool *failed)
{
	if (strings && path) {
		GLint success = false;
		const GLuint shader = glCreateShader(type);

		glShaderSource(shader, 1, strings, NULL);
		glCompileShader(shader);
		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
		if (success != true) {
			printf("Unable to compile %s %d! (Path: %s)\n", shader_enum_to_string(type), shader, path);
			printLog(shader, false);
			glDeleteShader(shader);
			*failed = true;
			return 0;
		}
		return shader;
	}
	return 0; //Means that shader is nonexistant, silently ignore.
}

static int compile_effect(const char *file_paths[], GLchar *shader_texts[], GLenum shader_types[], int num, GLuint program_handle)
{
	GLint success = true;
	bool failed = false;
	GLint shader_handles[num];

	for (int i = 0; i < num; i++)
		if ((shader_handles[i] = shader_new(shader_types[i], (const GLchar **)&shader_texts[i], file_paths[i], &failed)))
			glAttachShader(program_handle, shader_handles[i]);

	if (!failed) { //If we've successfully compiled all shaders (it's possible to fail compile but succeed linking, which is ugly.)
		glLinkProgram(program_handle);
		glGetProgramiv(program_handle, GL_LINK_STATUS, &success);
		if (!success) {
			printf("Unable to link program %d!\n", program_handle);
			printLog(program_handle, true);
		}
	}

	for (int i = 0; i < num; i++)
		glDeleteShader(shader_handles[i]);

	return !success; //Success is 0.
}

int compile_shader_program(GLuint program_handle, GLuint shaders[], int num_shaders)
{
	GLint success = true;

	for (int i = 0; i < num_shaders; i++)
		glAttachShader(program_handle, shaders[i]);

	if (success) {
		glLinkProgram(program_handle);
		glGetProgramiv(program_handle, GL_LINK_STATUS, &success);
		if (!success) {
			printf("Unable to link program %d!\n", program_handle);
			printLog(program_handle, true);
		}
	}
	return success;
}


static void init_attrs(GLuint program_handle, const char **astrs, GLint *attrs, int num)
{
	for (int i = 0; i < num; i++) {
		attrs[i] = glGetAttribLocation(program_handle, (const GLchar *)astrs[i]);
		if (attrs[i] == -1) {
			//printf("%s is not a valid vertex attribute! It may have been optimized out.\n", astrs[i]);
		}
	}
}

static void init_unifs(GLuint program_handle, const char **ustrs, GLint *unifs, int num)
{
	for (int i = 0; i < num; i++) {
		unifs[i] = glGetUniformLocation(program_handle, (const GLchar *)ustrs[i]);
		if (unifs[i] == -1) {
			//printf("%s is not a valid uniform variable! It may have been optimized out.\n", ustrs[i]);
		}
	}
}

void load_effects(
	EFFECT effects[],       int neffects,
	const char *paths[],    int npaths,
	const char *astrs[],    int nastrs,
	const char *ustrs[],    int nustrs)
{
	GLenum shader_types[] = {GL_VERTEX_SHADER, GL_GEOMETRY_SHADER, GL_FRAGMENT_SHADER};
	int nsh = LENGTH(shader_types); //Number of shader types.
	char *shader_texts[npaths];
	read_in_shaders(paths, shader_texts, npaths);

	for (int i = 0; i < neffects; i++) {
		GLuint program_handle = glCreateProgram();
		int offset = i*nsh;

		if (compile_effect(&paths[offset], &shader_texts[offset], shader_types, nsh, program_handle) == 0) {
			init_attrs(program_handle, astrs, effects[i].attr, nastrs);
			init_unifs(program_handle, ustrs, effects[i].unif, nustrs);
			glDeleteProgram(effects[i].handle); //Delete old program
			effects[i].handle = program_handle; //Store handle to new program
		} else {
			glDeleteProgram(program_handle);
			printf("Program [%s, %s] failed.\n", paths[i*nsh], paths[i*nsh + 2]);
		}
		//if (checkErrors("Compile an effect") != GL_NO_ERROR) printf("Effect %i produced an error. [%s]\n", i, paths[i*nsh]);
	}

	for (int i = 0; i < LENGTH(shader_texts); i++)
		if (shader_texts[i] != NULL)
			free(shader_texts[i]);
}
