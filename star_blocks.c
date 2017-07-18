#include <stdio.h>
#include <stdlib.h>
#include "math/bpos.h"
#include "math/utility.h"
#include "effects.h"
#include "entity/scriptable.h"
#include "entity/physical.h"
#include "gl_utils.h"

/*
Since each block has a bpos_origin, each star only needs to store its offset from that origin;
a uniform variable can represent the offset from the camera to the block origin, which is added
to each star position for the final camera-space star position.

Since a star will only ever be at most 2048 bpos_origins from the center of a star block, a 16-bit
integer vector will be sufficient for storing each star's position within the block.

I am concerned, however, that the distance from the camera to the block origin may be significant,
enough to incur float-precision issues, which might manifest non-uniformly, revealing the inherent
axes making up the universe.

I guess I'll just try it and see?
*/

extern float log_depth_intermediate_factor;
extern GLfloat proj_view_mat[16];

enum {
	STAR_BLOCK_SIZE = 4096, //The width of one edge of a star block, in bpos cell widths.
	STARS_PER_BLOCK = 10000, //The number of stars per "star block", of which there are 27.
};

static struct star_blocks_globals {
	GLuint vao;
	GLuint vbos[27];
	bpos_origin origins[3][3][3];
	int16_t stars[27][STARS_PER_BLOCK*3];
} star_blocks = {
	0
};

static bpos_origin star_blocks_nearest_origin(bpos_origin o)
{
	//Shift o by half a block width, and mask off some low-order bits.
	return (o + STAR_BLOCK_SIZE / 2) & ~(STAR_BLOCK_SIZE-1);
}

static void star_blocks_generate(bpos_origin o, int16_t *stars, int num)
{
	svec3 c1 = (svec3){0, 0, 0} - STAR_BLOCK_SIZE/2; //TODO: Check if I can do only "-STAR_BLOCK_SIZE/2"
	svec3 c2 = -c1;
	srand(((o.x * 13 + o.y) * 7 + o.z) * 53);
	for (int i = 0; i < num*3; i+=3) {
		svec3 star = rand_box_vec3(c1, c2);
		stars[i] = star.x;
		stars[i+1] = star.y;
		stars[i+2] = star.z;
	}
}

/*
Find the star block for the observer position.
For each block in 3x3x3 about that block, check that the origin matches.
Generate any block that does not have a matching origin.
*/

void star_blocks_update(bpos_origin observer)
{
	//Origin for the center star block.
	bpos_origin center = star_blocks_nearest_origin(observer);
	//Origin for the corner star block.
	bpos_origin cor = center - STAR_BLOCK_SIZE;
	//Indexes for the corner star block (center index + {-1, -1, -1}, wrapped around).
	qvec3 icor = ((center / STAR_BLOCK_SIZE) % 3 + 5) % 3; //TODO: Since this is all wrapped around, can I just use the center?
	glBindVertexArray(star_blocks.vao);

	/*
	Possible better approach: Find the index of the center block. For each neighbor of that, try generating
	and see if the origin already present in the array matches. Can I avoid a triply-nested loop? (And is it clearer?)
	*/

	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			for (int k = 0; k < 3; k++) {
				qvec3 indexes = (icor + (qvec3){i, j, k}) % 3;
				bpos_origin *sbo = &star_blocks.origins[indexes.x][indexes.y][indexes.z];
				bpos_origin curr = cor + (qvec3){i, j, k}*STAR_BLOCK_SIZE;
				if (sbo->x != curr.x || sbo->y != curr.y || sbo->z != curr.z) { //TODO: Assess if memcmp works better here.
					//Generate the stars for that star block.
					int tmp = i*9 + j*3 + k;
					*sbo = curr;
					star_blocks_generate(*sbo, star_blocks.stars[tmp], STARS_PER_BLOCK);
					glBindBuffer(GL_ARRAY_BUFFER, star_blocks.vbos[tmp]);
					glBufferData(GL_ARRAY_BUFFER, STARS_PER_BLOCK*sizeof(svec3), star_blocks.stars[tmp], GL_STATIC_DRAW);
					printf("Replacing block at "); qvec3_println(indexes);
					checkErrors("After buffering a star_block");
				}
			}
		}
	}
}

void star_blocks_init(bpos_origin observer)
{
	glUseProgram(effects.star_blocks.handle);
	checkErrors("Using program");
	glGenVertexArrays(1, &star_blocks.vao);
	checkErrors("gen vao");
	glBindVertexArray(star_blocks.vao);
	checkErrors("bind vao");
	glGenBuffers(27, star_blocks.vbos);
	checkErrors("After creating star_blocks buffers and vao");
	glEnableVertexAttribArray(effects.star_blocks.star_pos);
	glVertexAttribIPointer(effects.star_blocks.star_pos, 3, GL_SHORT, 0, NULL);
	glUniform1f(effects.star_blocks.bpos_size, BPOS_CELL_SIZE);
	glUniform1f(effects.star_blocks.log_depth_intermediate_factor, log_depth_intermediate_factor);
	checkErrors("After sending uniforms for star_blocks");
	star_blocks_update(observer);
	glBindVertexArray(0);
}

void star_blocks_draw(bpos_origin camera_origin)
{
	glBindVertexArray(star_blocks.vao);
	checkErrors("Bound the vao");
	glEnable(GL_BLEND); //We're going to blend the contribution from each individual star.
	glBlendEquation(GL_FUNC_ADD); //The light contributions get blended additively.
	glBlendFunc(GL_ONE, GL_ONE); //Just straight addition.
	glUseProgram(effects.star_blocks.handle);
	checkErrors("Set the program");
	glUniformMatrix4fv(effects.star_blocks.model_view_projection_matrix, 1, GL_TRUE, proj_view_mat);
	checkErrors("Sent star_blocks proj_view_mat");
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			for (int k = 0; k < 3; k++) {
				vec3 eye_block_offset = bpos_disp(camera_origin, star_blocks.origins[i][j][k]);
				glUniform3fv(effects.star_blocks.eye_block_offset, 1, (float *)&eye_block_offset);
				checkErrors("Sent eye_block_offset");
				int tmp = i*9 + j*3 + k;
				glBindBuffer(GL_ARRAY_BUFFER, star_blocks.vbos[tmp]);
				checkErrors("Bound the buffer");
				glDrawArrays(GL_POINTS, 0, STARS_PER_BLOCK);
				checkErrors("After drawing a block");
			}
		}
	}
	glDisable(GL_BLEND);
	glBindVertexArray(0);
}

scriptable_callback(star_blocks_script)
{
	Physical *camera = ((Entity *)entity->Scriptable->context)->Physical;
	star_blocks_update(camera->origin);
}