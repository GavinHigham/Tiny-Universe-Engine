#include "meter_ogl_renderer.h"
#include "meter.h"
#include "macros.h"
#include "math/utility.h"
#include "glsw/glsw.h"
#include "glsw_shaders.h"
#include "configuration/lua_configuration.h"
#include <stdio.h>
#include <stdlib.h> //TODO(Gavin): Check if I need these two
#include <string.h>

/* Lua Config */
extern lua_State *L;

struct meter_ogl_renderer_ctx meter_ogl_ctx = {0};
struct meter_renderer meter_ogl_renderer = {
	.init = meter_ogl_renderer_init,
	.deinit = meter_ogl_renderer_deinit,
	.render = meter_ogl_renderer_draw_all,
	.renderer_ctx = &meter_ogl_ctx
};

struct {unsigned short x, y;} glyph_coords[128];
static const char *character_set = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789.,;:?!-_~#\"'&()[]|`\\/@°+=*$£€<> ";
#define PRIMITIVE_RESTART_INDEX 0xFFFFFFFF
#define MAX_LABEL_LEN 512
float meter_value(struct meter *m);

struct meter_vertex {
	float pos[2];
	float tx[2];
	unsigned char color[4];
};

int meter_ogl_renderer_draw_all(struct meter_globals *meter)
{
	struct meter_ogl_renderer_ctx *ogl = meter->renderer.renderer_ctx;
	glBindVertexArray(ogl->vao);
	glDisable(GL_DEPTH_TEST);
	glUseProgram(ogl->shader);
	glBindBuffer(GL_ARRAY_BUFFER, ogl->vbo);
	glUniform2f(ogl->screen_res, meter->screen_width, meter->screen_height);
	glEnable(GL_PRIMITIVE_RESTART);
	glPrimitiveRestartIndex(PRIMITIVE_RESTART_INDEX);

	const int nov = 8;       //Num outer verts.
	const int vpm = nov + 4; //Verts per meter->
	const int ipm = 16;      //Indices per meter: 10 outer, primitive restart, 4 inner, primitive restart.
	int num_meter_verts = vpm * meter->num_meters;
	int char_quads_offset = num_meter_verts;
	int qs = ipm * meter->num_meters; //Quads start, keeps track of where new indices for text should be inserted.
	int qi = vpm * meter->num_meters; //Quads index, keeps track of next unused index for each successive quad.

	char label[MAX_LABEL_LEN];
	int total_label_chars = 0;
	for (int i = 0; i < meter->num_meters; i++) {
		struct meter *m = &meter->meters[i];
		total_label_chars += snprintf(NULL, 0, m->fmt, m->name, meter_value(m));
	}
	struct meter_vertex vbo[num_meter_verts + 4 * total_label_chars];
	unsigned int ibo[ipm * meter->num_meters + 5 * total_label_chars];


	/*
		Border vertices are arranged like this:
				0 ------------2
				|\           /|    And inner fill vertices are arranged like this: 
				| 1---------3 |                    8---------10
				| |         | |         ->         |         |
				| 7---------5 |                    9---------11
				|/           \|
				6-------------4    Inner vertices [8, 9, 10, 11] share the positions of border vertices [1, 7, 3, 5]
	*/

	//Create index buffer.
	unsigned int ibo_offsets[15] = {1, 0, 3, 2, 5, 4, 7, 6, 1, 0, PRIMITIVE_RESTART_INDEX, 8, 9, 10, 11};
	for (int i = 0; i < meter->num_meters; i++) {
		struct meter *m = &meter->meters[i];
		struct widget_meter_style s = m->style;
		int label_len = snprintf(label, sizeof(label), m->fmt, m->name, meter_value(m));
		
		//Create index buffer.
		for (int j = 0; j < ipm; j++)
			ibo[ipm*i + j] = ibo_offsets[j] + vpm*i;
		ibo[ipm*i + 10] = PRIMITIVE_RESTART_INDEX;
		ibo[ipm*i + 15] = PRIMITIVE_RESTART_INDEX;
		//Create indices for glyphs.
		while (qs < LENGTH(ibo)) {
			memcpy(&ibo[qs], (unsigned int[]){qi, qi+1, qi+2, qi+3, PRIMITIVE_RESTART_INDEX}, 5 * sizeof(unsigned int));
			qs += 5;
			qi += 4;
		}

		//Create vertex buffer.
		float scale = (meter_value(m)-m->min) / (m->max - m->min);
		float positions[] = {
			m->x,                                                 m->y,            
			m->x + s.padding,                                     m->y + s.padding,            
			m->x + s.width,                                       m->y,            
			m->x + s.padding + (s.width - 2 * s.padding) * scale, m->y + s.padding,            
			m->x + s.width,                                       m->y + s.height,
			m->x + s.padding + (s.width - 2 * s.padding) * scale, m->y - s.padding + s.height,
			m->x,                                                 m->y + s.height,
			m->x + s.padding,                                     m->y - s.padding + s.height,
		};

		for (int j = 0; j < nov; j++) {
			memcpy(vbo[vpm*i + j].pos, &positions[2*j], 2 * sizeof(float));
			memcpy(vbo[vpm*i + j].color, m->color.border, 4 * sizeof(unsigned char));
			memset(vbo[vpm*i + j].tx, 0, 2*sizeof(float));
		}

		//Fill fill positions and colors.
		int map[] = {1, 7, 3, 5};
		for (int j = 0; j < 4; j++) {
			memcpy(vbo[vpm*i + j + nov].pos,   &positions[2*map[j]], 2 * sizeof(float));
			memcpy(vbo[vpm*i + j + nov].color, m->color.fill,        4 * sizeof(unsigned char));
			memset(vbo[vpm*i + j + nov].tx, 0, 2*sizeof(float));
		}

		float w = ogl->font.width, h = ogl->font.height, y_offset = (int)((s.height - ogl->font.height)/2.0) + 1;
		for (int j = 0; j < label_len; j++) {
			int label_char = label[j];
			float gx = glyph_coords[label_char].x;
			float gy = glyph_coords[label_char].y;
			struct meter_vertex quad[] = {
				{{m->x + 2 * s.padding + w * j,     m->y + y_offset},     {gx,     gy}},
				{{m->x + 2 * s.padding + w * (j+1), m->y + y_offset},     {gx + w, gy}},
				{{m->x + 2 * s.padding + w * j,     m->y + y_offset + h}, {gx,     gy + h}},
				{{m->x + 2 * s.padding + w * (j+1), m->y + y_offset + h}, {gx + w, gy + h}}
			};
			for (int k = 0; k < 4; k++)
				memcpy(&quad[k].color, m->color.font, sizeof(m->color.font));
			memcpy(&vbo[char_quads_offset], quad, sizeof(quad));
			char_quads_offset += 4;
		}
	}

	glBufferData(GL_ARRAY_BUFFER, sizeof(vbo), vbo, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ogl->ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(ibo), ibo, GL_DYNAMIC_DRAW);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, ogl->font_tex);
	glUniform1i(ogl->font_tex_unif, 0);

	glUniform1i(ogl->textured_unif, false);
	glDrawElements(GL_TRIANGLE_STRIP, ipm * meter->num_meters, GL_UNSIGNED_INT, NULL);

	//Draw labels
	glUniform1i(ogl->textured_unif, true);
	glDrawElements(GL_TRIANGLE_STRIP, 5 * total_label_chars, GL_UNSIGNED_INT, (GLvoid *)(sizeof(unsigned int) * ipm * meter->num_meters));

	glBindVertexArray(0);
	return 0;
}

int meter_ogl_renderer_deinit(struct meter_globals *meter)
{
	struct meter_ogl_renderer_ctx *ogl = meter->renderer.renderer_ctx;
	glDeleteVertexArrays(1, &ogl->vao);
	glDeleteBuffers(1, &ogl->vbo);
	glDeleteProgram(ogl->shader);
	glDeleteTextures(1, &ogl->font_tex);
	return 0;
}

int meter_ogl_renderer_init(struct meter_globals *meter)
{
	struct meter_ogl_renderer_ctx *ogl = meter->renderer.renderer_ctx;
	glGenVertexArrays(1, &ogl->vao);
	glBindVertexArray(ogl->vao);
	glGenBuffers(1, &ogl->vbo);
	glGenBuffers(1, &ogl->ibo);

	/* Shader initialization */
	glswInit();
	glswSetPath("shaders/glsw/", ".glsl");
	glswAddDirectiveToken("GL33", "#version 330");

	char texture_path[gettmpglobstr(L, "meter_font", "4x7.png", NULL)];
	                  gettmpglobstr(L, "meter_font", "4x7.png", texture_path);

	GLuint shader[] = {
		glsw_shader_from_keys(GL_VERTEX_SHADER, "meter.vertex.GL33"),
		glsw_shader_from_keys(GL_FRAGMENT_SHADER, "meter.fragment.GL33"),
	};
	ogl->shader = glsw_new_shader_program(shader, LENGTH(shader));
	glswShutdown();

	if (!ogl->shader)
		goto error;

	ogl->screen_res    = glGetUniformLocation(ogl->shader, "screen_res");
	ogl->font_tex_unif = glGetUniformLocation(ogl->shader, "font_tex");
	ogl->textured_unif = glGetUniformLocation(ogl->shader, "textured");
	/* Vertex attributes */

	//TODO(Gavin): Just use layout specifier thing in the shader.
	ogl->pos_attr = glGetAttribLocation(ogl->shader, "pos");
	ogl->col_attr = glGetAttribLocation(ogl->shader, "col");
	ogl->tx_attr  = glGetAttribLocation(ogl->shader, "tx");

	glBindBuffer(GL_ARRAY_BUFFER, ogl->vbo);
	glEnableVertexAttribArray(ogl->pos_attr);
	glVertexAttribPointer(ogl->pos_attr, 2, GL_FLOAT, GL_FALSE, sizeof(struct meter_vertex), 0);
	glEnableVertexAttribArray(ogl->tx_attr);
	glVertexAttribPointer(ogl->tx_attr, 2, GL_FLOAT, GL_FALSE, sizeof(struct meter_vertex), (void *)offsetof(struct meter_vertex, tx));
	glEnableVertexAttribArray(ogl->col_attr);
	glVertexAttribPointer(ogl->col_attr, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(struct meter_vertex), (void *)offsetof(struct meter_vertex, color));

	//TODO(Gavin) Load these from a font configuration file.
	ogl->font_tex = load_gl_texture(texture_path);
	ogl->font.width = 6;
	ogl->font.height = 12;

	for (int i = 0; i < 128; i++) {
		//'?' location.
		glyph_coords[i].x = 84;
		glyph_coords[i].y = 24;
	}

	int len = strlen(character_set);
	for (int i = 0; i < len; i++) {
		glyph_coords[(int)character_set[i]].x = ogl->font.width * (i % 26);
		glyph_coords[(int)character_set[i]].y = ogl->font.height * (i / 26);
	}

	// glDisable(GL_DEPTH_TEST);
	glBindVertexArray(0);

	return 0;

error:
	return -1;
}