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

static int init_shader_attributes(GLuint program_handle, struct shader_prog *program, struct shader_info info)
{
	for (int i = 0; i < LENGTH(program->attr); i++) {
		if (program->attr[i] != -1) {
			glBindAttribLocation(program_handle, i, info.attr_names[i]);
			program->attr[i] = i;
			if (program->attr[i] == -1) {
				printf("%s is not a valid glsl program variable! It may have been optimized out.\n", info.attr_names[i]);
				return -1;
			}
		}
	}
	return 0;
}

static int init_shader_uniforms(GLuint program_handle, struct shader_prog *program, struct shader_info info)
{
	for (int i = 0; i < LENGTH(program->unif); i++) {
		if (program->unif[i] != -1) {
			program->unif[i] = glGetUniformLocation(program_handle, info.unif_names[i]);
			if (program->unif[i] == -1) {
				program->unif[i] = 0;
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

char *shader_enum_to_string(GLenum shader_type)
{
	switch (shader_type) {
	case GL_VERTEX_SHADER:   return "vertex shader";
	case GL_FRAGMENT_SHADER: return "fragment shader";
	case GL_GEOMETRY_SHADER: return "geometry shader";
	default: return "unhandled shader type";
	}
}

static int compile_and_attach_shader(GLenum type, const GLchar **source, const char *path, GLuint program_handle)
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
		glAttachShader(program_handle, shader);
		return shader;
	}
	return 0;
}

static int init_shader_program(GLuint program_handle,
	const GLchar **vs_source, const char *vs_path,
	const GLchar **fs_source, const char *fs_path,
	const GLchar **gs_source, const char *gs_path)
{
	GLint success = true;
	const GLuint vshader = compile_and_attach_shader(GL_VERTEX_SHADER, vs_source, vs_path, program_handle);
	const GLuint fshader = compile_and_attach_shader(GL_FRAGMENT_SHADER, fs_source, fs_path, program_handle);
	const GLuint gshader = compile_and_attach_shader(GL_GEOMETRY_SHADER, gs_source, gs_path, program_handle);
	
	if (vshader && fshader) {
		glLinkProgram(program_handle);
		glGetProgramiv(program_handle, GL_LINK_STATUS, &success);
		if (success != true) {
			printf("Unable to link program %d! (%s, %s, %s)\n", program_handle, vs_path, fs_path, gs_path);
			printLog(program_handle, true);
		}
	}

	if (vshader) glDeleteShader(vshader);
	if (fshader) glDeleteShader(fshader);
	if (gshader) glDeleteShader(gshader);
	if (!success) return -1;
	return 0;
}

static int init_shader(GLuint program_handle, struct shader_info info, bool reload)
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

		int error = init_shader_program(program_handle,
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
	} else {
		if (init_shader_program(program_handle,
			(const GLchar **)info.vs_source, *info.vs_file_path,
			(const GLchar **)info.fs_source, *info.fs_file_path,
			(const GLchar **)info.gs_source, info.gs_file_path?*info.gs_file_path:NULL)) {
			printf("Could not compile shader program.\n");
			return -1;
		}
	}
	return 0;
}

static int init_or_reload_shaders(struct shader_prog **programs, struct shader_info **infos, int numprogs, bool reload)
{
	for (int i = 0; i < numprogs; i++) {
		bool success = true;
		GLuint program_handle = glCreateProgram();
		if (init_shader_attributes(program_handle, programs[i], *infos[i])) {
			printf("Could not bind shader attributes for program.\n");
			success = false;
		}
		int result = init_shader(program_handle, *infos[i], reload);
		if (result) {
			char *errstr = reload?"reload":"init";
			printf("Error: could not %s shader %i, aborting %s.\n", errstr, i, errstr);
			success = false;
		}
		if (init_shader_uniforms(program_handle, programs[i], *infos[i])) {
			printf("Could not retrieve shader uniforms.\n");
			success = false;
		}
		if (success) {
			if (reload)
				glDeleteProgram(programs[i]->handle);
			programs[i]->handle = program_handle;
		} else {
			glDeleteProgram(program_handle);
			printf("Stopped at program [%s, %s]\n", *infos[i]->vs_file_path, *infos[i]->fs_file_path);
			return -1;
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