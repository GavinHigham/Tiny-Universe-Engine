#include <gl/glew.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include "glla.h"
#include "macros.h"
#include "effects.h"
#include "math/utility.h"
#include "math/space_sector.h"

const int NUM_STARS = 40000;
const float STAR_SECTOR_RADIUS = 10000;
const float STAR_RADIUS = 10000;
//Bias the star distance out this much to move them away from the planet
const float STAR_BIAS = STAR_RADIUS/2.0;

GLuint stars_vbo;
GLuint stars_vao;
extern amat4 inv_eye_frame;
extern amat4 eye_frame;
extern amat4 ship_frame;
extern GLfloat proj_view_mat[16];
extern float far_distance;
extern float near_distance;

space_sector star_buffer[NUM_STARS];
int32_t nearby_stars[3 * NUM_STARS];
int actual_num_stars;

void init_stars()
{
	srand(101);
	glUseProgram(effects.stars.handle);
	//glUniformMatrix4fv(effects.stars.projection_matrix, 1, GL_TRUE, proj_mat);

	//Make a cube that encompasses our sphere of stars.
	//Create random points in the cube, discard those not also in sphere.
	//Cube volume: 8*radius^3, Sphere volume: (4/3)*pi*r^3
	//Point is outside of sphere volume (1-((4/3)*pi)/8) or 47.6% of the time.
	//Corners of the volume within which sectors will be chosen.
	vec3 c1 = {-STAR_SECTOR_RADIUS, -STAR_SECTOR_RADIUS, -STAR_SECTOR_RADIUS};
	vec3 c2 = {STAR_SECTOR_RADIUS, STAR_SECTOR_RADIUS, STAR_SECTOR_RADIUS};
	// //Corners of the sector within which the star will be placed in sector-local coordinates.
	// vec3 sc1 = {-POSITIONAL_ANCHOR_SECTOR_SIZE, -POSITIONAL_ANCHOR_SECTOR_SIZE, -POSITIONAL_ANCHOR_SECTOR_SIZE};
	// vec3 sc2 = {-POSITIONAL_ANCHOR_SECTOR_SIZE, -POSITIONAL_ANCHOR_SECTOR_SIZE, -POSITIONAL_ANCHOR_SECTOR_SIZE};
	for (int i = 0; i < NUM_STARS; i++) {
		//Choose a random sector to put the star in.
		vec3 s = rand_box_point3d(c1, c2);
		//If the star is in the sphere, keep it.
		if (vec3_mag(s) < STAR_SECTOR_RADIUS * STAR_SECTOR_RADIUS) {
			//s += s * (STAR_BIAS/vec3_mag(s));
			//vec3 p = rand_box_point3d(sc1, sc2); //If I want to move the star around within the sector.
			star_buffer[i] = (space_sector){s.x, s.y, s.z};
		}
		else {
			i--;
		}
		// vec3 p = rand_bunched_point3d_in_sphere((vec3){{0,0,0}}, STAR_RADIUS);
		//star_buffer[i] = (vec3){{p.x, p.y, p.z/10}};
	}

	actual_num_stars = 0;
	for (int i = 0; i < NUM_STARS; i++) {
		space_sector s = star_buffer[i];
		int32_t x = s.x;
		int32_t y = s.y;
		int32_t z = s.z;
		//Check that an int32_t can hold the value safely
		if (x == s.x && y == s.y && z == s.z) {
			nearby_stars[i*3]     = x;
			nearby_stars[i*3 + 1] = y;
			nearby_stars[i*3 + 2] = z;
			actual_num_stars++;
		}
	}

	glGenVertexArrays(1, &stars_vao);
	glBindVertexArray(stars_vao);
	glGenBuffers(1, &stars_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, stars_vbo);
	glBufferData(GL_ARRAY_BUFFER, actual_num_stars*sizeof(int32_t), nearby_stars, GL_STATIC_DRAW);
	glEnableVertexAttribArray(effects.stars.sector_coords);
	glVertexAttribPointer(effects.stars.sector_coords, 3, GL_INT, GL_FALSE, 0, NULL);
	glUniform1f(effects.stars.sector_size, SPACE_SECTOR_SIZE);
	glUniform1f(effects.stars.log_depth_intermediate_factor, 2.0/log(far_distance/near_distance));
	glUniform1f(effects.stars.near_plane_dist, near_distance);
}

void deinit_stars()
{
	glDeleteVertexArrays(1, &stars_vao);
	glDeleteBuffers(1, &stars_vbo);
}

void draw_stars()
{
	glEnable(GL_BLEND); //We're going to blend the contribution from each individual star.
	glBlendEquation(GL_FUNC_ADD); //The light contributions get blended additively.
	glBlendFunc(GL_ONE, GL_ONE); //Just straight addition.
	glBindVertexArray(stars_vao);
	glUseProgram(effects.stars.handle);
	//glUniform3fv(effects.stars.ship_position, 1, eye_frame.T);
	glUniform3fv(effects.stars.eye_pos, 1, (float *)&eye_frame.t);
	glUniform3i(effects.stars.eye_sector_coords, 0, 0, 0);
	//glUniform1f(effects.stars.stars_radius, STAR_RADIUS+STAR_BIAS);
	glUniformMatrix4fv(effects.stars.model_view_projection_matrix, 1, GL_TRUE, proj_view_mat);
	glDrawArrays(GL_POINTS, 0, actual_num_stars);
	glDisable(GL_BLEND);
	glBindVertexArray(0);
}