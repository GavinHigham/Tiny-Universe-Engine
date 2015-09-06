#ifndef INIT_H
#define INIT_H

//Forward declarations. Defined in "shaders.h"
struct shader_prog;
struct shader_info;

int init();
void deinit();
void checkErrors(char *label);

#endif