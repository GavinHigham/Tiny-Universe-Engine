#ifndef SHADER_UTILS_H
#define SHADER_UTILS_H

#include "effects.h"
#include <stdbool.h>

struct shader_prog;
struct shader_info;

void load_effects(
	EFFECT effects[], int neffects,
	const char *paths[],    int npaths,
	const char *astrs[],    int nastrs,
	const char *ustrs[],    int nustrs);

void printLog(GLuint handle, bool is_program);
GLuint shader_new(GLenum type, const GLchar **strings, const char *path);
int compile_shader_program(GLuint program_handle, GLuint shaders[], int num_shaders);


int init_effects(struct shader_prog **programs, struct shader_info **infos, int numprogs);
int reload_effects(struct shader_prog **programs, struct shader_info **infos, int numprogs);

#endif