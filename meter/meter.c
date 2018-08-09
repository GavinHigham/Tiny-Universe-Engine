#include "meter.h"
#include "glsw/glsw.h"
#include "glsw_shaders.h"
#include "macros.h"
#include "math/utility.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <GL/glew.h>

struct meter {
	char *name;
	float x, y, width, height, min, max, value, padding, *target;
	unsigned char fill_color[4], border_color[4], font_color[4];
	meter_callback_fn callback;
	void *callback_context;
};

static struct meter_globals {
	float screen_width, screen_height;
	unsigned int num_meters;
	unsigned int max_meters;
	struct meter *meters;
	enum meter_state state;
	char *clicked_meter_name;
	float clicked_x, clicked_y;
	int total_label_chars;
	struct {
		GLuint shader, vao, vbo, ibo, screen_res, font_tex, font_tex_unif, textured_unif;
		GLint pos_attr, col_attr, tx_attr;
	} ogl;
	struct {
		float width, height;
	} font;
} meter = {0};

struct meter_vertex {
	float pos[2];
	float tx[2];
	unsigned char color[4];
};

struct {unsigned short x, y;} glyph_coords[128];
static const char *character_set = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789.,;:?!-_~#\"'&()[]|`\\/@°+=*$£€<> ";
#define PRIMITIVE_RESTART_INDEX 0xFFFFFFFF

int meter_OpenGL_draw_all()
{
	glBindVertexArray(meter.ogl.vao);
	glDisable(GL_DEPTH_TEST);
	glUseProgram(meter.ogl.shader);
	glBindBuffer(GL_ARRAY_BUFFER, meter.ogl.vbo);
	glUniform2f(meter.ogl.screen_res, meter.screen_width, meter.screen_height);
	glEnable(GL_PRIMITIVE_RESTART);
	glPrimitiveRestartIndex(PRIMITIVE_RESTART_INDEX);

	const int nov = 8;       //Num outer verts.
	const int vpm = nov + 4; //Verts per meter.
	const int ipm = 16;      //Indices per meter: 10 outer, primitive restart, 4 inner, primitive restart.
	int num_meter_verts = vpm * meter.num_meters;
	int char_quads_offset = num_meter_verts;
	int qs = ipm * meter.num_meters; //Quads start, keeps track of where new indices for text should be inserted.
	int qi = vpm * meter.num_meters; //Quads index, keeps track of next unused index for each successive quad.
	struct meter_vertex vbo[num_meter_verts + 4 * meter.total_label_chars];
	unsigned int ibo[ipm * meter.num_meters + 5 * meter.total_label_chars];

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
	for (int i = 0; i < meter.num_meters; i++) {
		struct meter *m = &meter.meters[i];
		int name_len = strlen(m->name);
		
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
		float scale = m->value / (m->max - m->min);
		float positions[] = {
			m->x,                                                    m->y,            
			m->x + m->padding,                                       m->y + m->padding,            
			m->x + m->width,                                         m->y,            
			m->x + m->padding + (m->width - 2 * m->padding) * scale, m->y + m->padding,            
			m->x + m->width,                                         m->y + m->height,
			m->x + m->padding + (m->width - 2 * m->padding) * scale, m->y - m->padding + m->height,
			m->x,                                                    m->y + m->height,
			m->x + m->padding,                                       m->y - m->padding + m->height,
		};

		for (int j = 0; j < nov; j++) {
			memcpy(vbo[vpm*i + j].pos, &positions[2*j], 2 * sizeof(float));
			memcpy(vbo[vpm*i + j].color, m->border_color, 4 * sizeof(unsigned char));
			memset(vbo[vpm*i + j].tx, 0, 2*sizeof(float));
		}

		//Fill fill positions and colors.
		int map[] = {1, 7, 3, 5};
		for (int j = 0; j < 4; j++) {
			memcpy(vbo[vpm*i + j + nov].pos,   &positions[2*map[j]], 2 * sizeof(float));
			memcpy(vbo[vpm*i + j + nov].color, m->fill_color,        4 * sizeof(unsigned char));
			memset(vbo[vpm*i + j + nov].tx, 0, 2*sizeof(float));
		}

		float w = meter.font.width, h = meter.font.height, y_offset = (int)((m->height - meter.font.height)/2.0) + 1;
		for (int j = 0; j < name_len; j++) {
			float gx = glyph_coords[(int)m->name[j]].x;
			float gy = glyph_coords[(int)m->name[j]].y;
			struct meter_vertex quad[] = {
				{{m->x + 2 * m->padding + w * j,     m->y + y_offset},     {gx,     gy}},
				{{m->x + 2 * m->padding + w * (j+1), m->y + y_offset},     {gx + w, gy}},
				{{m->x + 2 * m->padding + w * j,     m->y + y_offset + h}, {gx,     gy + h}},
				{{m->x + 2 * m->padding + w * (j+1), m->y + y_offset + h}, {gx + w, gy + h}}
			};
			for (int k = 0; k < 4; k++)
				memcpy(&quad[k].color, m->font_color, sizeof(m->font_color));
			memcpy(&vbo[char_quads_offset], quad, sizeof(quad));
			char_quads_offset += 4;
		}
	}

	glBufferData(GL_ARRAY_BUFFER, sizeof(vbo), vbo, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meter.ogl.ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(ibo), ibo, GL_DYNAMIC_DRAW);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, meter.ogl.font_tex);
	glUniform1i(meter.ogl.font_tex_unif, 0);

	glUniform1i(meter.ogl.textured_unif, false);
	glDrawElements(GL_TRIANGLE_STRIP, ipm * meter.num_meters, GL_UNSIGNED_INT, NULL);

	//Draw labels
	glUniform1i(meter.ogl.textured_unif, true);
	glDrawElements(GL_TRIANGLE_STRIP, 5 * meter.total_label_chars, GL_UNSIGNED_INT, (GLvoid *)(sizeof(unsigned int) * ipm * meter.num_meters));

	glBindVertexArray(0);
	return 0;
}

void meter_OpenGL_deinit()
{
	glDeleteVertexArrays(1, &meter.ogl.vao);
	glDeleteBuffers(1, &meter.ogl.vbo);
	glDeleteProgram(meter.ogl.shader);
	glDeleteTextures(1, &meter.ogl.font_tex);
}

static int meter_OpenGL_init()
{
	glGenVertexArrays(1, &meter.ogl.vao);
	glBindVertexArray(meter.ogl.vao);
	glGenBuffers(1, &meter.ogl.vbo);
	glGenBuffers(1, &meter.ogl.ibo);

	/* Shader initialization */
	glswInit();
	glswSetPath("shaders/glsw/", ".glsl");
	glswAddDirectiveToken("GL33", "#version 330");

	GLuint shader[] = {
		glsw_shader_from_keys(GL_VERTEX_SHADER, "meter.vertex.GL33"),
		glsw_shader_from_keys(GL_FRAGMENT_SHADER, "meter.fragment.GL33"),
	};
	meter.ogl.shader = glsw_new_shader_program(shader, LENGTH(shader));
	glswShutdown();

	if (!meter.ogl.shader)
		goto error;

	meter.ogl.screen_res    = glGetUniformLocation(meter.ogl.shader, "screen_res");
	meter.ogl.font_tex_unif = glGetUniformLocation(meter.ogl.shader, "font_tex");
	meter.ogl.textured_unif = glGetUniformLocation(meter.ogl.shader, "textured");
	/* Vertex attributes */

	//TODO(Gavin): Just use layout specifier thing in the shader.
	meter.ogl.pos_attr = glGetAttribLocation(meter.ogl.shader, "pos");
	meter.ogl.col_attr = glGetAttribLocation(meter.ogl.shader, "col");
	meter.ogl.tx_attr  = glGetAttribLocation(meter.ogl.shader, "tx");

	glBindBuffer(GL_ARRAY_BUFFER, meter.ogl.vbo);
	glEnableVertexAttribArray(meter.ogl.pos_attr);
	glVertexAttribPointer(meter.ogl.pos_attr, 2, GL_FLOAT, GL_FALSE, sizeof(struct meter_vertex), 0);
	glEnableVertexAttribArray(meter.ogl.tx_attr);
	glVertexAttribPointer(meter.ogl.tx_attr, 2, GL_FLOAT, GL_FALSE, sizeof(struct meter_vertex), (void *)offsetof(struct meter_vertex, tx));
	glEnableVertexAttribArray(meter.ogl.col_attr);
	glVertexAttribPointer(meter.ogl.col_attr, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(struct meter_vertex), (void *)offsetof(struct meter_vertex, color));

	//TODO(Gavin) Load these from a font configuration file.
	meter.ogl.font_tex = load_gl_texture("4x7.png");//load_gl_texture("grass.png");
	//printf("\n%i\n\n", meter.ogl.font_tex);
	meter.font.width = 6;
	meter.font.height = 12;

	for (int i = 0; i < 128; i++) {
		//'?' location.
		glyph_coords[i].x = 84;
		glyph_coords[i].y = 24;
	}

	int len = strlen(character_set);
	for (int i = 0; i < len; i++) {
		glyph_coords[(int)character_set[i]].x = meter.font.width * (i % 26);
		glyph_coords[(int)character_set[i]].y = meter.font.height * (i / 26);
	}

	// glDisable(GL_DEPTH_TEST);
	glBindVertexArray(0);

	return 0;

error:
	return -1;
}

static int meter_check_enclosing(struct meter *m, float x, float y)
{
	return x >= m->x && y >= m->y && x <= (m->x + m->width) && y <= (m->y + m->height);
}

//Find the first meter that encloses x and y.
static int meter_find_enclosing(float x, float y)
{
	for (int i = 0; i < meter.num_meters; i++)
		if (meter_check_enclosing(&meter.meters[i], x, y))
			return i;
	return -1;
}

static int meter_get_index(char *name)
{
	for (int i = 0; i < meter.num_meters; i++)
		if (!strcmp(name, meter.meters[i].name))
			return i;
	return -1;
}

#define METER_GET_INDEX_OR_RET_ERROR(name, index_name) int index_name = meter_get_index(name); if (mi < 0) return mi;

static void meter_update(struct meter *m, float value)
{
	m->value = value;
	if (m->target)
		*(m->target) = m->value;
	if (m->callback)
		m->callback(m->name, meter.state, m->value, m->callback_context);
}

//Add a new meter. Returns 0 on success.
int meter_add(char *name, float width, float height, float min, float value, float max)
{
	if (meter.num_meters >= meter.max_meters)
		return -1;

	meter.total_label_chars += strlen(name);
	meter.meters[meter.num_meters++] = (struct meter){
		.name = strdup(name),
		.x = 0, //CHANGE THIS
		.y = 0, //CHANGE THIS
		.width = width,
		.height = height,
		.min = min,
		.max = max,
		.value = value,
		.padding = 3.0,
		.target = NULL,
		.fill_color = {187, 187, 187, 255},
		.border_color = {95, 95, 95, 255},
		.font_color = {255, 255, 255},
		.callback = NULL,
		.callback_context = NULL,
	};
	return 0;
}

//Change an existing meter. Returns 0 on success.
int meter_change(char *name, float width, float height, float min, float max, float value)
{
	METER_GET_INDEX_OR_RET_ERROR(name, mi)

	struct meter *m = &meter.meters[mi];
	m->width = width;
	m->height = height;
	m->min = min;
	m->max = max;
	m->value = value;

	return 0;
}

//Move an existing meter to position x, y. Not relative to current position.
int meter_position(char *name, float x, float y)
{
	METER_GET_INDEX_OR_RET_ERROR(name, mi)

	meter.meters[mi].x = x;
	meter.meters[mi].y = y;	
	return 0;
}

//Set a callback to be called when an existing meter is clicked, dragged, or released.
//Callback arguments:
// name: name of the meter.
// drag_state: Enum telling if callback was called on click start, drag, or click end.
// value: current value of the meter.
// context: the same context that was provided to meter_callback(...) on callback registration.
int meter_callback(char *name, meter_callback_fn callback, void *context)
{
	METER_GET_INDEX_OR_RET_ERROR(name, mi)

	meter.meters[mi].callback = callback;
	meter.meters[mi].callback_context = context;	
	return 0;
}

int meter_target(char *name, float *target)
{
	METER_GET_INDEX_OR_RET_ERROR(name, mi)

	struct meter *m = &meter.meters[mi];
	m->target = target;
	meter_update(m, m->value);
	return 0;
}

//Add a new meter with the same characteristics as an existing meter.
int meter_duplicate(char *name, char *duplicate_name)
{
	METER_GET_INDEX_OR_RET_ERROR(name, mi)

	meter.meters[meter.num_meters++] = meter.meters[mi];
	meter.meters[meter.num_meters - 1].name = strdup(name);
	return 0;
}

//Change the style of an existing meter.
int meter_style(char *name, unsigned char fill_color[4], unsigned char border_color[4], unsigned char font_color[4], float padding)
{
	METER_GET_INDEX_OR_RET_ERROR(name, mi)

	memcpy(meter.meters[mi].fill_color,   fill_color,   4 * sizeof(unsigned char));
	memcpy(meter.meters[mi].border_color, border_color, 4 * sizeof(unsigned char));
	memcpy(meter.meters[mi].font_color,   font_color,   4 * sizeof(unsigned char));
	return 0;
}

//Return the internal value of an existing meter, or 0 on error.
float meter_get(char *name)
{
	int mi = meter_get_index(name);
	if (mi < 0)
		return 0;

	return meter.meters[mi].value;
}

//Fill value with the internal value of an existing meter.
int meter_get_value(char *name, float *value)
{
	METER_GET_INDEX_OR_RET_ERROR(name, mi)

	*value = meter.meters[mi].value;
	return 0;
}

//Delete an existing meter.
int meter_delete(char *name)
{
	METER_GET_INDEX_OR_RET_ERROR(name, mi)

	meter.total_label_chars -= strlen(meter.meters[mi].name);
	free(meter.meters[mi].name);
	//Shift meters down, taking the place of the deleted meter.
	memmove(&meter.meters[mi], &meter.meters[mi+1], sizeof(struct meter) * (meter.num_meters - mi - 1));
	meter.num_meters--;
	return 0;
}

//Inform the meter module of current mouse state. Should be done in update function.
//Returns 1 if any meter was clicked.
int meter_mouse(float x, float y, bool mouse_down)
{
	if (meter.state == METER_CLICK_STARTED || meter.state == METER_DRAGGED) {
		//PERF(Gavin): If I want to make this faster in the future, I can directly compare the char pointers,
		//since I store the actual pointer of the clicked meter, instead of a duplicate.
		int mi = meter_get_index(meter.clicked_meter_name);
		if (mi < 0) {
			//Got into a weird state, correct it.
			meter.state = METER_CLICK_ENDED;
			meter.clicked_meter_name = NULL;
			return 0;
		}

		if (mouse_down)
			meter.state = METER_DRAGGED;
		else
			meter.state = METER_CLICK_ENDED;

		struct meter *m = &meter.meters[mi];
		float pad_x_left = m->x + m->padding;
		float pad_x_right = m->x + m->width - 2 * m->padding;
		float value = (x - pad_x_left) / (pad_x_right - pad_x_left) * (m->max - m->min);
		value = fmax(m->min, fmin(value, m->max));
		meter_update(m, value);
		printf("Meter dragged, set to %f\n", m->value);
	} else if (meter.state == METER_CLICK_ENDED) {
		if (mouse_down) {
			int mi = meter_find_enclosing(x, y);
			if (mi < 0)
				return 0;

			meter.state = METER_CLICK_STARTED;
			struct meter *m = &meter.meters[mi];
			meter.clicked_meter_name = m->name;
			meter.clicked_x = x;
			meter.clicked_y = y;

			//Pass a different value if I want absolute instead of relative sliding.
			meter_update(m, m->value);
		} else {
			//Do nothing.
		}
	}
	return 0;
}

//Draw all meters.
int meter_draw_all()
{
	return meter_OpenGL_draw_all();
}

void meter_glyph_meter_callback(char *name, enum meter_state state, float value, void *context)
{
	float *target = context;
	*target = (int)value;
}

//Init the meter system.
int meter_init(float screen_width, float screen_height, unsigned int max_num_meters)
{
	meter = (struct meter_globals){
		.screen_width = screen_width,
		.screen_height = screen_height,
		.num_meters = 0,
		.max_meters = max_num_meters,
		.meters = malloc(max_num_meters * sizeof(struct meter))
	};

	if (meter_OpenGL_init() < 0) {
		meter_deinit();
		goto error;
	}

	// //TODO(Gavin): Tweak these, position these
	// meter_add("Font Width", 100, 20, 0.0, 6.0, 100.0);
	// meter_callback("Font Width", meter_glyph_meter_callback, &meter.font.width);
	// // meter_target("Font Width", &meter.font.width);
	// meter_style("Font Width", (unsigned char[]){79, 79, 207, 255}, (unsigned char[]){47, 47, 95, 255}, (unsigned char[]){255, 255, 255, 255}, 2.0);
	// meter_position("Font Width", 5.0, 70.0);

	// meter_add("Font Height", 100, 20, 0.0, 12.0, 100.0);
	// // meter_target("Font Height", &meter.font.height);
	// meter_callback("Font Height", meter_glyph_meter_callback, &meter.font.height);
	// meter_style("Font Height", (unsigned char[]){79, 79, 207, 255}, (unsigned char[]){47, 47, 95, 255}, (unsigned char[]){255, 255, 255, 255}, 2.0);
	// meter_position("Font Height", 5.0, 95.0);

	return 0;
error:
	printf("Error initializing meter.\n");
	return -1;
}

//Deinit the meter system.
int meter_deinit()
{
	for (int i = 0; i < meter.num_meters; i++)
		free(meter.meters[i].name);
	free(meter.meters);

	meter_OpenGL_deinit();
	return 0;
}

//Inform the meter module of a screen resize.
int meter_resize_screen(float screen_width, float screen_height)
{
	meter.screen_width = screen_width;
	meter.screen_height = screen_height;
	//Possibly reflow meters?
	return 0;
}