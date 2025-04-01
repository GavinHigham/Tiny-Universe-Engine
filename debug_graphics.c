#include "debug_graphics.h"
#include "graphics.h"
#include "macros.h"
#include "math/utility.h"
#include "math/bpos.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>

//Externs that could be useful.
extern amat4 inv_eye_frame;
extern amat4 ship_frame;
extern float far_distance;
extern float near_distance;
extern bpos_origin eye_sector;
extern float log_depth_intermediate_factor;

//Global state data I might want to access outside.
struct debug_graphics_globals debug_graphics = {
	.is_init = false,
};

void debug_graphics_init()
{
	if (debug_graphics.is_init)
		return;
	glGenVertexArrays(1, &debug_graphics.vao);
	checkErrors("debug_graphics vao gen");
	glBindVertexArray(debug_graphics.vao);
	checkErrors("debug_graphics vao bind");

	glUseProgram(effects.debug_graphics.handle);
	checkErrors("debug_graphics use program");
	glGenBuffers(1, &debug_graphics.vbo);
	checkErrors("debug_graphics gen buffers");
	glUniform1f(effects.debug_graphics.log_depth_intermediate_factor, log_depth_intermediate_factor);
	checkErrors("debug_graphics gl uniform");

	//Whatever setup I need for debug graphics. Can derive this from the similar stars init.

	glBindVertexArray(0);
	checkErrors("bind vao 0");
	debug_graphics.is_init = true;
}

void debug_graphics_deinit()
{
	if (!debug_graphics.is_init)
		return;

	glDeleteVertexArrays(1, &debug_graphics.vao);
	glDeleteBuffers(1, &debug_graphics.vbo);

	debug_graphics.is_init = false;
}

int buffer_lines()
{
	glEnableVertexAttribArray(effects.debug_graphics.vPos);
	glVertexAttribPointer(effects.debug_graphics.vPos, 3, GL_FLOAT, GL_FALSE, 6*sizeof(GLfloat), NULL);
	glEnableVertexAttribArray(effects.debug_graphics.vColor);
	glVertexAttribPointer(effects.debug_graphics.vColor, 3, GL_FLOAT, GL_FALSE, 6*sizeof(GLfloat), (void *)(3*sizeof(GLfloat)));

	GLfloat lines[] = { //Position, color, interleaved vertex attributes.
		VEC3_COORDS(debug_graphics.lines.ship_to_planet.start), 0.0, 0.8, 0.0,
		VEC3_COORDS(debug_graphics.lines.ship_to_planet.end),   0.0, 0.1, 0.0
	};

	glBufferData(GL_ARRAY_BUFFER, sizeof(lines), lines, GL_DYNAMIC_DRAW);

	return 2; //Current number of vertices.
}

int buffer_points()
{
	glEnableVertexAttribArray(effects.debug_graphics.vPos);
	glVertexAttribPointer(effects.debug_graphics.vPos, 3, GL_FLOAT, GL_FALSE, 6*sizeof(GLfloat), NULL);
	glEnableVertexAttribArray(effects.debug_graphics.vColor);
	glVertexAttribPointer(effects.debug_graphics.vColor, 3, GL_FLOAT, GL_FALSE, 6*sizeof(GLfloat), (void *)(3*sizeof(GLfloat)));

	GLfloat points[] = { //Position, color, interleaved vertex attributes.
		VEC3_COORDS(debug_graphics.points.ship_to_planet_intersection.pos), 0.0, 0.3, 0.9,
	};

	glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_DYNAMIC_DRAW);

	return 1; //Current number of vertices.
}

void debug_graphics_draw(amat4 eye_frame, float proj_view_mat[16])
{
	if (!debug_graphics.is_init)
		return;

	glBindVertexArray(debug_graphics.vao);
	glUseProgram(effects.debug_graphics.handle);
	glUniform3f(effects.debug_graphics.eye_pos, eye_frame.t.x, eye_frame.t.y, eye_frame.t.z);
	glUniformMatrix4fv(effects.debug_graphics.model_view_projection_matrix, 1, GL_TRUE, proj_view_mat);

	//I can just reallocate the buffer every time I want to draw a different kind of debug graphics primitive.
	//For simplicity, they don't need to run fast.

	if (debug_graphics.lines.ship_to_planet.enabled) {
		glBindBuffer(GL_ARRAY_BUFFER, debug_graphics.vbo);
		int num = buffer_lines();
		glDrawArrays(GL_LINES, 0, num);
	}

	if (debug_graphics.points.ship_to_planet_intersection.enabled) {
		glBindBuffer(GL_ARRAY_BUFFER, debug_graphics.vbo);
		int num = buffer_points();
		glDrawArrays(GL_POINTS, 0, num);
	}

	glBindVertexArray(0);
	glUseProgram(0);
}