#ifndef INIT_H
#define INIT_H

//Forward declarations. Defined in "shaders.h"
struct shader_prog;
struct shader_info;

int init();
void deinit();
void checkErrors(char *label);
int init_shaders(struct shader_prog **programs, struct shader_info **infos, int numprogs, int reload);

#endif