#ifndef RENDER_H
#define RENDER_H
#include <GL/glew.h>
#include "func_list.h"

void update(float dt); //should be moved to its own source file when things get big
void render();
void init_render();
void deinit_render();
void make_projection_matrix(GLfloat fov, GLfloat a, GLfloat n, GLfloat f, GLfloat *buf, int buf_len);
void setup_attrib_for_draw(GLuint attr_handle, GLuint buffer, GLenum attr_type, int attr_size);
extern struct func_list update_func_list;

#endif