#include <gl/glew.h>
#include <math.h>
#include <stdlib.h>
#include "math/vector3.h"
#include "math/affine_matrix4.h"
#include "shaders/shaders.h"
#include "macros.h"

#define NUM_STARS 1000000
#define STAR_RADIUS 1000

GLuint stars_vbo;
GLuint stars_vao;
extern AMAT4 inv_eye_frame;
extern AMAT4 eye_frame;
extern AMAT4 ship_frame;
extern GLfloat proj_mat[16];

vec3 star_buffer[NUM_STARS];

float rand_float()
{
	return (float)((double)rand()/RAND_MAX); //Discard precision after the division.
}

vec3 rand_bunched_point3d_in_sphere(vec3 origin, float radius)
{
	radius = pow(rand_float(), 4) * radius; //Distribute stars within the sphere, not on the outside.
	float a1 = rand_float() * 2 * M_PI;
	float a2 = rand_float() * 2 * M_PI;
	return vec3_add(origin, (vec3){{radius*sin(a1)*cos(a2), radius*sin(a1)*sin(a2), radius*cos(a1)}});
}

vec3 rand_box_point3d(vec3 corner1, vec3 corner2)
{
	vec3 size = vec3_sub(corner2, corner1); //size can be negative, it'll still work.
	return (vec3){{
		corner1.x + rand_float()*size.x,
		corner1.y + rand_float()*size.y,
		corner1.z + rand_float()*size.z
	}};
}

void init_stars()
{
	srand(101);
	glUseProgram(stars_program.handle);
	glUniformMatrix4fv(stars_program.projection_matrix, 1, GL_TRUE, proj_mat);

	vec3 c1 = {{-STAR_RADIUS, -STAR_RADIUS, -STAR_RADIUS}};
	vec3 c2 = {{STAR_RADIUS, STAR_RADIUS, STAR_RADIUS}};
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
	amat4_to_array(mvm_buf, LENGTH(mvm_buf), inv_eye_frame);
	glUniformMatrix4fv(stars_program.model_view_matrix, 1, GL_TRUE, mvm_buf);
	glDrawArrays(GL_POINTS, 0, NUM_STARS);
	glDisable(GL_BLEND);
}