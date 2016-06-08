#include <gl/glew.h>
#include <math.h>
#include <stdlib.h>
#include <glalgebra.h>
#include "math/utility.h"
#include "shaders/shaders.h"
#include "macros.h"

#define NUM_STARS 1000000
#define STAR_RADIUS 1000

GLuint stars_vbo;
GLuint stars_vao;
extern amat4 inv_eye_frame;
extern amat4 eye_frame;
extern amat4 ship_frame;
extern GLfloat proj_mat[16];

vec3 star_buffer[NUM_STARS];

void init_stars()
{
	srand(101);
	glUseProgram(stars_program.handle);
	//glUniformMatrix4fv(stars_program.projection_matrix, 1, GL_TRUE, proj_mat);

	//vec3 c1 = {{-STAR_RADIUS, -STAR_RADIUS, -STAR_RADIUS}};
	//vec3 c2 = {{STAR_RADIUS, STAR_RADIUS, STAR_RADIUS}};
	//Cube volume: 8*radius^3, Sphere volume: (4/3)*pi*r^3
	//Point is outside of sphere volume (1-((4/3)*pi)/8) or 47.6% of the time
	for (int i = 0; i < NUM_STARS; i++) {
		// vec3 p = rand_box_point3d(c1, c2);
		// if (p.x*p.x + p.y*p.y + p.z*p.z < STAR_RADIUS * STAR_RADIUS)
		// 	star_buffer[i] = p;
		// else
		// 	i--;
		vec3 p = rand_bunched_point3d_in_sphere((vec3){{0,0,0}}, STAR_RADIUS);
		star_buffer[i] = (vec3){{p.x, p.y, p.z/10}};
	}

	glGenVertexArrays(1, &stars_vao);
	glBindVertexArray(stars_vao);
	glGenBuffers(1, &stars_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, stars_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(star_buffer), star_buffer, GL_STATIC_DRAW);
	glEnableVertexAttribArray(stars_program.vPos);
	glVertexAttribPointer(stars_program.vPos, 3, GL_FLOAT, GL_FALSE, 0, NULL);
}

void deinit_stars()
{
	glDeleteBuffers(1, &stars_vbo);
}

void draw_stars()
{
	glEnable(GL_BLEND); //We're going to blend the contribution from each individual star.
	glBlendEquation(GL_FUNC_ADD); //The light contributions get blended additively.
	glBlendFunc(GL_ONE, GL_ONE); //Just straight addition.
	glBindVertexArray(stars_vao);
	glUseProgram(stars_program.handle);
	//glUniform3fv(stars_program.ship_position, 1, eye_frame.T);
	glUniform3fv(stars_program.eye_pos, 1, eye_frame.T);
	GLfloat mvm_buf[16];
	//Send model_view_matrix.
	amat4_to_array(inv_eye_frame, mvm_buf);
	glUniformMatrix4fv(stars_program.model_view_projection_matrix, 1, GL_TRUE, mvm_buf);
	glDrawArrays(GL_POINTS, 0, NUM_STARS);
	glDisable(GL_BLEND);
}