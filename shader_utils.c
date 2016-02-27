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

static int init_shader_attributes(struct shader_prog *program, struct shader_info info)
{
	for (int i = 0; i < LENGTH(program->attr); i++) {
		if (program->attr[i] != -1) {
			program->attr[i] = glGetAttribLocation(program->handle, info.attr_names[i]);
			if (program->attr[i] == -1) {
				printf("%s is not a valid glsl program variable! It may have been optimized out.\n", info.attr_names[i]);
				return -1;
			}
		}
	}
	return 0;
}

static int init_shader_uniforms(struct shader_prog *program, struct shader_info info)
{
	for (int i = 0; i < LENGTH(program->unif); i++) {
		if (program->unif[i] != -1) {
			program->unif[i] = glGetUniformLocation(program->handle, info.unif_names[i]);
			if (program->unif[i] == -1) {
				printf("%s is not a valid glsl uniform variable! It may have been optimized out.\n", info.unif_names[i]);
				return -1;
			}
		}
	}
	return 0;
}

void printLog(GLuint handle, bool is_program)
{
	//Why write two log printing functions when you can use FUNCTION POINTERS AND TERNARY OPERATORS >:D
	GLboolean (*glIsLOGTYPE)(GLuint)                                  = is_program? glIsProgram         : glIsShader;
	void (*glGetLOGTYPEiv)(GLuint, GLenum, GLint *)                   = is_program? glGetProgramiv      : glGetShaderiv;
	void (*glGetLOGTYPEInfoLog)(GLuint, GLsizei, GLsizei *, GLchar *) = is_program? glGetProgramInfoLog : glGetShaderInfoLog;

	if (glIsLOGTYPE(handle)) {
		int log_length = 0;
		int max_length = log_length;
		glGetLOGTYPEiv(handle, GL_INFO_LOG_LENGTH, &max_length);
		char *info_log = (char *)malloc(max_length);
		glGetLOGTYPEInfoLog(handle, max_length, &log_length, info_log);
		if (log_length > 0) {
			printf("%s\n", info_log);
		}
		free(info_log);
	} else {
		printf("Name %d is not a %s.\n", handle, is_program?"program":"shader");
	}
}

static int init_shader_program(struct shader_prog *program,
	const GLchar **vs_source, const char *vs_path,
	const GLchar **fs_source, const char *fs_path,
	const GLchar **gs_source, const char *gs_path)
{
	program->handle = glCreateProgram();
	const GLuint vshader = glCreateShader(GL_VERTEX_SHADER);
	const GLuint fshader = glCreateShader(GL_FRAGMENT_SHADER);
	const GLuint gshader = glCreateShader(GL_GEOMETRY_SHADER);
	//Vertex shader part
	glShaderSource(vshader, 1, vs_source, NULL);
	glCompileShader(vshader);
	GLint success = false;
	glGetShaderiv(vshader, GL_COMPILE_STATUS, &success);
	if (success != true) {
		printf("Unable to compile vertex shader %d! (Path: %s)\n", vshader, vs_path);
		printLog(vshader, false);
		return -1;
	}
	glAttachShader(program->handle, vshader);
	//Fragment shader part
	glShaderSource(fshader, 1, fs_source, NULL);
	glCompileShader(fshader);
	success = false;
	glGetShaderiv(fshader, GL_COMPILE_STATUS, &success);
	if (success != true) {
		printf("Unable to compile fragment shader %d! (Path: %s)\n", fshader, fs_path);
		printLog(fshader, false);
		return -1;
	}
	glAttachShader(program->handle, fshader);
	//Geometry shader part, optional
	if (gs_source && gs_path) {
		glShaderSource(gshader, 1, gs_source, NULL);
		glCompileShader(gshader);
		GLint success = false;
		glGetShaderiv(gshader, GL_COMPILE_STATUS, &success);
		if (success != true) {
			printf("Unable to compile vertex shader %d! (Path: %s)\n", gshader, gs_path);
			printLog(gshader, false);
			return -1;
		}
		glAttachShader(program->handle, gshader);
	}
	glLinkProgram(program->handle);
	success = true;
	glGetProgramiv(program->handle, GL_LINK_STATUS, &success);
	if (success != true) {
		printf("Unable to link program %d! (%s, %s, %s)\n", program->handle, vs_path, fs_path, gs_path);
		printLog(program->handle, true);
		return -1;
	}
	glDeleteShader(vshader);
	glDeleteShader(fshader);
	glDeleteShader(gshader);
	return 0;
}

static int init_shader(struct shader_prog *program, struct shader_info info, bool reload)
{
	if (reload) {
		//Delete existing program
		int fd;
		struct stat buf;
		//Load in the vertex shader
		fd = open(*info.vs_file_path, O_RDONLY);
		fstat(fd, &buf);
		GLchar *vs_source = (GLchar *) malloc(buf.st_size+1);
		read(fd, vs_source, buf.st_size);
		close(fd);
		vs_source[buf.st_size] = '\0';
		//Load in the fragment shader
		fd = open(*info.fs_file_path, O_RDONLY);
		fstat(fd, &buf);
		GLchar *fs_source = (GLchar *) malloc(buf.st_size+1);
		read(fd, fs_source, buf.st_size);
		close(fd);
		fs_source[buf.st_size] = '\0';
		//Load in the geometry shader, it's optional
		GLchar *gs_source = NULL;
		if (info.gs_file_path) {
			fd = open(*info.gs_file_path, O_RDONLY);
			fstat(fd, &buf);
			gs_source = (GLchar *) malloc(buf.st_size+1);
			read(fd, gs_source, buf.st_size);
			close(fd);
			gs_source[buf.st_size] = '\0';
		}

		struct shader_prog program_copy = *program;
		int error = init_shader_program(&program_copy,
			(const GLchar **)&vs_source, *info.vs_file_path,
			(const GLchar **)&fs_source, *info.fs_file_path,
			(const GLchar **)&gs_source, info.gs_file_path?*info.gs_file_path:NULL);
		free(vs_source);
		free(fs_source);
		if (info.gs_file_path)
			free(gs_source);
		if (error) {
			printf("Could not compile shader program.\n");
			return -1;
		}
		glDeleteProgram(program->handle);
		*program = program_copy;
	} else if (init_shader_program(program,
		(const GLchar **)info.vs_source, *info.vs_file_path,
		(const GLchar **)info.fs_source, *info.fs_file_path,
		(const GLchar **)info.gs_source, info.gs_file_path?*info.gs_file_path:NULL)) {
		printf("Could not compile shader program.\n");
		return -1;
	}
	if (init_shader_attributes(program, info)) {
		printf("Could not retrieve shader attributes.\n");
		return -1;
	}
	if (init_shader_uniforms(program, info)) {
		printf("Could not retrieve shader uniforms.\n");
		return -1;
	}
	return 0;
}

static int init_or_reload_shaders(struct shader_prog **programs, struct shader_info **infos, int numprogs, bool reload)
{
	for (int i = 0; i < numprogs; i++) {
		int result = init_shader(programs[i], *infos[i], reload);
		if (result) {
			char *errstr = reload?"reload":"init";
			printf("Error: could not %s shader %i, aborting %s.\n", errstr, i, errstr);
			return result;
		}
	}
	return 0;
}

int init_shaders(struct shader_prog **programs, struct shader_info **infos, int numprogs)
{
	return init_or_reload_shaders(programs, infos, numprogs, false);
}

int reload_shaders(struct shader_prog **programs, struct shader_info **infos, int numprogs)
{
	return init_or_reload_shaders(programs, infos, numprogs, true);
}