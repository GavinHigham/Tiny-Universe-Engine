#ifndef DEBUG_GRAPHICS_H
#define DEBUG_GRAPHICS_H
#include "glla.h"
#include "graphics.h"
#include <stdbool.h>

struct debug_graphics_line {
	vec3 start;
	vec3 end;
	bool enabled;
};

struct debug_graphics_point {
	vec3 pos;
	bool enabled;
};

extern struct debug_graphics_globals {
	GLuint vbo;
	GLuint vao;
	bool is_init;

	struct {
		struct debug_graphics_line ship_to_planet;
		struct debug_graphics_line tile[3];
	} lines;

	struct {
		struct debug_graphics_point ship_to_planet_intersection;
	} points;
} debug_graphics;

void debug_graphics_init();
void debug_graphics_deinit();
void debug_graphics_draw();

#endif