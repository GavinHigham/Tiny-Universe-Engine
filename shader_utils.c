#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <GL/glew.h>
#include <assert.h>
#include "macros.h"
#include "shaders/shaders.h"
#include "gl_utils.h"

static int init_effect_attributes(GLuint program_handle, struct shader_prog *program, struct shader_info info)
{
	for (int i = 0; i < LENGTH(program->attr); i++) {
		if (program->attr[i] != -1) {
			glBindAttribLocation(program_handle, i, effect_attribute_names[i]);
			program->attr[i] = i;
			if (program->attr[i] == -1) {
				printf("%s is not a valid glsl program variable! It may have been optimized out.\n", effect_attribute_names[i]);
				return -1;
			}
		}
	}
	return 0;
}

//static void init_effect_uniforms(GLuint program_handle, struct shader_prog *program, struct shader_info info)
static void init_effect_uniforms(GLuint program_handle, GLint unif_names[], const GLchar *unif_strnames[], int numunifs)
{
	for (int i = 0; i < numunifs; i++)
		unif_names[i] = glGetUniformLocation(program_handle, unif_strnames[i]);
}

void printLog(GLuint handle, bool is_program)
{
	//Why write two log printing functions when you can use FUNCTION POINTERS AND TERNARY OPERATORS >:D
	GLboolean (*glIs)(GLuint)                                  = is_program? glIsProgram         : glIsShader;
	void (*glGetiv)(GLuint, GLenum, GLint *)                   = is_program? glGetProgramiv      : glGetShaderiv;
	void (*glGetInfoLog)(GLuint, GLsizei, GLsizei *, GLchar *) = is_program? glGetProgramInfoLog : glGetShaderInfoLog;

	if (glIs(handle)) {
		int log_length = 0;
		int max_length = log_length;
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

char *shader_enum_to_string(GLenum shader_type)
{
	switch (shader_type) {
	case GL_VERTEX_SHADER:   return "vertex shader";
	case GL_FRAGMENT_SHADER: return "fragment shader";
	case GL_GEOMETRY_SHADER: return "geometry shader";
	default: return "unhandled shader type";
	}
}

static GLuint compile_shader(GLenum type, const GLchar **source, char *path, GLuint program_handle)
{
	if (source && path) {
		const GLuint shader = glCreateShader(type);
		glShaderSource(shader, 1, source, NULL);
		glCompileShader(shader);
		GLint success = false;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
		if (success != true) {
			printf("Unable to compile %s %d! (Path: %s)\n", shader_enum_to_string(type), shader, path);
			printLog(shader, false);
			glDeleteShader(shader);
			return 0;
		}
		return shader;
	}
	return 0;
}

//Each array member of shader_texts should be free()'d if this function returns 0.
static int read_in_shaders(char *file_paths[], GLchar **shader_texts[], int num)
{
	int fd, i;
	struct stat buf;
	for (i = 0; i < num; i++) {
		fd = open(file_paths[i], O_RDONLY);
		if (fd < 0) {
			printf("Could not open file: %s\n", file_paths[i]);
			goto fail;
		}
		fstat(fd, &buf);
		*shader_texts[i] = (GLchar *) malloc(buf.st_size+1);
		if (*shader_texts[i] == NULL)
			goto fail;
		read(fd, shader_texts[i], buf.st_size);
		close(fd);
		(*shader_texts[i])[buf.st_size] = '\0';
	}
	return 0;

fail:
	for (int j = 0; j < i; j++)
		if (*shader_texts[j] != NULL)
			free(*shader_texts[j]);
	if (fd >= 0)
		close(fd);
	return -1;
}

static int compile_effect(char *file_paths[], GLchar **shader_texts[], GLenum shader_types[], int num, GLuint program_handle)
{
	GLuint shader_handles[num];
	GLint success = true;
	int i;
	for (i = 0; i < num; i++) {
		shader_handles[i] = compile_shader(shader_types[i], (const GLchar **)shader_texts[i], file_paths[i], program_handle);
		glAttachShader(program_handle, shader_handles[i]);
	}
	glLinkProgram(program_handle);
	glGetProgramiv(program_handle, GL_LINK_STATUS, &success);
	if (success != true) {
		printf("Unable to link program %d!\n", program_handle);
		printLog(program_handle, true);
	}
	for (int j = 0; j < i; j++)
		if (shader_handles[j] != 0)
			glDeleteShader(shader_handles[j]);
	return !success;
}

static int init_or_reload_effects(struct shader_prog **programs, struct shader_info **infos, int numprogs, bool reload)
{
	for (int i = 0; i < numprogs; i++) {
		bool success = true;
		GLuint program_handle = glCreateProgram();
		if (init_effect_attributes(program_handle, programs[i], *infos[i])) {
			printf("Could not bind shader attributes for program.\n");
			success = false;
		}
		//int result = init_effect(program_handle, *infos[i], reload);
		int result;
		GLenum shader_types[] = {GL_VERTEX_SHADER, GL_GEOMETRY_SHADER, GL_FRAGMENT_SHADER};
		int num = LENGTH(shader_types);
		char **shader_texts[MAX_SHADERS_PER_EFFECT];
		if (reload) {
			read_in_shaders((char **)infos[i]->file_paths, shader_texts, num);
			result = compile_effect((char **)*infos[i]->file_paths, shader_texts, shader_types, num, program_handle);
		} else {
			result = compile_effect((char **)*infos[i]->file_paths, (GLchar ***)infos[i]->shader_texts, shader_types, num, program_handle);
		}
		if (reload)
			for (int j = 0; j < num; j++)
				if (*shader_texts[j] != NULL)
					free(*shader_texts[j]);
		if (result) {
			char *errstr = reload?"reload":"init";
			printf("Error: could not %s shader %i, aborting %s.\n", errstr, i, errstr);
			success = false;
		}
		//To do: Use a different variable to get the number of uniforms for the last argument here.
		init_effect_uniforms(program_handle, programs[i]->unif, effect_uniform_names, LENGTH(programs[i]->unif));

		if (success) {
			if (reload)
				glDeleteProgram(programs[i]->handle);
			programs[i]->handle = program_handle;
		} else {
			glDeleteProgram(program_handle);
			printf("Stopped at program [%s, %s]\n", *infos[i]->file_paths[0], *infos[i]->file_paths[2]);
			return -1;
		}
	}
	return 0;
}

int init_effects(struct shader_prog **programs, struct shader_info **infos, int numprogs)
{
	return init_or_reload_effects(programs, infos, numprogs, false);
}

int reload_effects(struct shader_prog **programs, struct shader_info **infos, int numprogs)
{
	return init_or_reload_effects(programs, infos, numprogs, true);
}