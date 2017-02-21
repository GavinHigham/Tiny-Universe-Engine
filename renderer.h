#ifndef RENDERER_H
#define RENDERER_H
#include <GL/glew.h>
#include "func_list.h"

extern float screen_width;
extern float screen_height;
void update(float dt); //should be moved to its own source file when things get big
void render();
void renderer_init();
void renderer_deinit();
void handle_resize(int screen_width, int screen_height);
void make_projection_matrix(GLfloat fov, GLfloat a, GLfloat n, GLfloat f, GLfloat *buf, int buf_len);
extern struct func_list update_func_list;

#endif