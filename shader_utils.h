#ifndef SHADER_UTILS_H
#define SHADER_UTILS_H

#include "effects.h"

struct shader_prog;
struct shader_info;

void load_effects(
	EFFECT effects[], int neffects,
	const char *paths[],    int npaths,
	const char *astrs[],    int nastrs,
	const char *ustrs[],    int nustrs);

int init_effects(struct shader_prog **programs, struct shader_info **infos, int numprogs);
int reload_effects(struct shader_prog **programs, struct shader_info **infos, int numprogs);

#endif