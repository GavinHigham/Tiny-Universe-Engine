#ifndef METER_OGL_RENDERER_H
#define METER_OGL_RENDERER_H

#include "meter.h"
#include <GL/glew.h>

struct meter_ogl_renderer_ctx {
	GLuint shader, vao, vbo, ibo, screen_res, font_tex, font_tex_unif, textured_unif;
	GLint pos_attr, col_attr, tx_attr;
	struct {
		float width, height;
	} font;
};
struct meter_renderer meter_ogl_renderer;

int meter_ogl_renderer_init(struct meter_globals *meter);
int meter_ogl_renderer_deinit(struct meter_globals *meter);
int meter_ogl_renderer_draw_all(struct meter_globals *meter);

#endif