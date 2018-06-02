#include "star_box.h"
#include "../math/bpos.h"
#include "../math/utility.h"
#include "../effects.h"
#include "../entity/scriptable.h"
#include "../entity/physical.h"
#include "../math/utility.h"
#include "../macros.h"
#include "../open-simplex-noise-in-c/open-simplex-noise.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <GL/glew.h>

/*
Since each box has a bpos_origin, each star only needs to store its offset from that origin;
a uniform variable can represent the offset from the camera to the box origin, which is added
to each star position for the final camera-space star position.

A star will only ever be at most 2048 bpos_origins from the center of a star box, so a 16-bit
integer vector would be sufficient for storing each star's position within the box.

I am concerned, however, that the distance from the camera to the box origin may be significant,
enough to incur float-precision issues, which might manifest non-uniformly, revealing the inherent
axes making up the universe.

I guess I'll just try it and see?
Epilogue: It worked!
*/

extern float log_depth_intermediate_factor;

//HALF_BOX_SIZE and -HALF_BOX_SIZE need to be representable in an int32_t.
const int64_t STAR_BOX_SIZE = 67108864;//4294967296; //The width of one edge of a star box, in bpos cell widths.
const int64_t HALF_BOX_SIZE = STAR_BOX_SIZE / 2;
enum {
	//TODO: Consider if I can just keep this small and multiply in the vertex shader.
	STARS_PER_BOX = 10000, //The number of stars per "star box", of which there are NUM_BOXES.
	NUM_BOXES = 27,
	BUCKET_DIVS_PER_AXIS = 16,
	BUCKETS_PER_BOX = BUCKET_DIVS_PER_AXIS*BUCKET_DIVS_PER_AXIS*BUCKET_DIVS_PER_AXIS
};

const size_t STAR_SIZE = sizeof(int32_t) * 3;

static struct star_box_globals {
	GLuint         vaos[NUM_BOXES];
	GLuint         vbos[NUM_BOXES];
	bpos_origin origins[NUM_BOXES];
	int32_t       stars[NUM_BOXES*3*STARS_PER_BOX];
	int   start_indices[NUM_BOXES * BUCKETS_PER_BOX];
} star_box = {
	{0}
};

static bpos_origin star_box_truncate_origin(bpos_origin o, int64_t precision)
{
	//Shift o by half a box width, and mask off some low-order bits.
	//Will this cause endian-ness errors? (Do I care?)
	return (o + precision / 2) & ~(precision-1);
}

int bucket_idx(int32_t star[3])
{
	int64_t d = STAR_BOX_SIZE/BUCKET_DIVS_PER_AXIS;
	return ((star[0]+HALF_BOX_SIZE)/d + (star[1]+HALF_BOX_SIZE)/d*3 + (star[2]+HALF_BOX_SIZE)/d*9);
}

static void star_box_generate(bpos_origin o, int box)
{
	vec3 c1 = -HALF_BOX_SIZE;
	vec3 c2 =  HALF_BOX_SIZE;
	srand(hash_qvec3(o));
	int32_t *stars = &star_box.stars[STARS_PER_BOX * 3 * box];
	int *buckets = &star_box.start_indices[BUCKETS_PER_BOX * box];
	//The number of stars in each bucket
	int bucket_counts[BUCKETS_PER_BOX] = {0};

	for (int i = 0; i < STARS_PER_BOX; i++) {
		vec3 v = rand_box_vec3(c1, c2);
		int32_t *star = &stars[3*i];
		star[0] = v.x; star[1] = v.y; star[2] = v.z;
		//memcpy(star, &v, STAR_SIZE);
		//Increment bucket, each bucket holds the number of stars it contains.
		bucket_counts[bucket_idx(star)]++;
	}

	//Make each bucket store its start index
	int start = 0;
	for (int i = 0; i < BUCKETS_PER_BOX; i++) {
		buckets[i] = start;
		start += bucket_counts[i];
	}

	//Sort stars into buckets
	//Pick a starting star
	int32_t star[3] = {stars[0], stars[1], stars[2]};
	for (int i = 0; i < STARS_PER_BOX; i++) {
		//Find where it goes.
		int bucket = bucket_idx(star);
		if (bucket < 0 || bucket >= BUCKETS_PER_BOX)
			printf("Index bad for star with position {%i, %i, %i}", star[0], star[1], star[2]);
		assert(bucket < BUCKETS_PER_BOX);
		assert(bucket >= 0);
		int32_t *dest = &stars[buckets[bucket] + bucket_counts[bucket]--];
		//Save the contents of dest
		int32_t tmp_star[3];
		memcpy(tmp_star, dest, STAR_SIZE);
		//Overwrite dest with the previous saved contents
		memcpy(dest, star, STAR_SIZE);
		//Save the star that was overwritten for the next iteration.
		memcpy(star, tmp_star, STAR_SIZE);
	}
}

static int idx_from_pt(bpos_origin pt)
{
	qvec3 idx;
	for (int i = 0; i < 3; i++) {
		idx[i] = lldiv_floor(pt[i], STAR_BOX_SIZE).quot % 3;
		idx[i] += 3*(idx[i] < 0);
	}
	return idx.x + idx.y * 3 + idx.z * 9;
}

void bucket_from_pt(bpos_origin pt, int *box, int *bucket)
{
	*box = idx_from_pt(star_box_truncate_origin(pt, STAR_BOX_SIZE));
	qvec3 tmp = pt - star_box.origins[*box]; //pt relative to box origin.
	*bucket = bucket_idx((int32_t[3]){tmp.x, tmp.y, tmp.z});
}

static void nearest_star_in_bucket(int *buckets, int bucket, ivec3 pt, int *star_idx, float *closest_dist)
{
	for (int i = buckets[bucket]; i < buckets[bucket+1]; i += 3) {
		float star_dist = ivec3_distf(pt, star_box.stars[i]);
		if (star_dist < *closest_dist) {
			*closest_dist = star_dist;
			*star_idx = i;
		}
	}
}

//Make this slow and branchy at first, can optimize later.
int star_box_find_nearest_star_idx(bpos_origin pt)
{
	/*
	We start by searching b, the bucket p falls into, finding the nearest s.
	Construct a sphere centered around p, just large enough to touch s.
	Any bucket b' that the sphere intersects must be searched as well, as there
	could be a nearer star in b'.

	Let a = (s - p).
	For each cardinal direction d,
		Let p' = |a|d + p. If p' lies in a different bucket, search that bucket.
	For each corner of b, c, let d = (c - p)/|c - p|.
		Let p' = |a|d + p. If p' lies in a different bucket, search that bucket.
	*/

	ivec3 ipt = {VEC3_COORDS(pt)};
	int box, bucket, spill_box, spill_bucket, star_idx = -1;
	float closest_dist = INFINITY;
	bucket_from_pt(pt, &box, &bucket);
	int *buckets = &star_box.start_indices[BUCKETS_PER_BOX * box];
	nearest_star_in_bucket(buckets, bucket, ipt, &star_idx, &closest_dist);
	printf("Box: %i, bucket: %i\n", box, bucket);

	qvec3 cardinals[] = {
		{ 0,  0,  1},
		{ 0,  1,  0},
		{ 1,  0,  0},
		{ 0,  0, -1},
		{ 0, -1,  0},
		{-1,  0,  0}
	};

	for (int i = 0; i < LENGTH(cardinals); i++) {
		bucket_from_pt(pt + cardinals[i]*(int64_t)closest_dist, &spill_box, &spill_bucket);
		if (spill_bucket == bucket && spill_box == box)
			continue;

		int *spill_buckets = &star_box.start_indices[BUCKETS_PER_BOX * spill_box];
		nearest_star_in_bucket(spill_buckets, spill_bucket, ipt, &star_idx, &closest_dist);
	}

	return star_idx;
}

#if 0
qvec3 star_box_get_star_origin(int star_idx)
{
	//Find the box index given the star index
	//Look up the star box origin
	//Add the star to the origin and return
	int32_t *s = &star_box.stars[star_idx];
}
#endif

/*
Find the star box for the observer position.
For each box in 3x3x3 about that box, check that the origin matches.
Generate any box that does not have a matching origin.
*/

void star_box_update(bpos_origin observer)
{
	bpos_origin center = star_box_truncate_origin(observer, STAR_BOX_SIZE);
	bool any_changed = false;

	for (int i = 0; i < NUM_BOXES; i++) {
		bpos_origin curr = center + ((qvec3){i%3, (i/3)%3, (i/9)} - 1)*STAR_BOX_SIZE;
		int idx = idx_from_pt(curr);
		bpos_origin sbo = star_box.origins[idx];
		if (memcmp(&sbo, &curr, sizeof(sbo))) {
			any_changed = true;
			//printf("Replacing box at %i, new origin ", i); qvec3_print(curr); puts("");
			//Generate the stars for that star box.
			star_box.origins[idx] = curr;
			star_box_generate(star_box.origins[idx], idx);
			glBindBuffer(GL_ARRAY_BUFFER, star_box.vbos[idx]);
			glBufferData(GL_ARRAY_BUFFER, STAR_SIZE*STARS_PER_BOX, &star_box.stars[idx*3*STARS_PER_BOX], GL_STATIC_DRAW);
		}
	}

	if (any_changed) {
		//printf("Updating stars, new center at "); qvec3_print(center); puts("");
	}
}

void star_box_init(bpos_origin observer)
{
	srand(101);
	glGenVertexArrays(NUM_BOXES, star_box.vaos);
	glGenBuffers(NUM_BOXES, star_box.vbos);
	for (int i = 0; i < NUM_BOXES; i++) {
		glBindVertexArray(star_box.vaos[i]);
		glEnableVertexAttribArray(effects.star_box.star_pos);
		glBindBuffer(GL_ARRAY_BUFFER, star_box.vbos[i]);
		glVertexAttribPointer(effects.star_box.star_pos, 3, GL_INT, GL_FALSE, 0, NULL);
		star_box.origins[i] = observer + 42; //Subtle bug: An initial 0,0,0 origin would not be generated.
	}
	star_box_update(observer);
	glUseProgram(effects.star_box.handle);
	glUniform1f(effects.star_box.star_box_size, STAR_BOX_SIZE);
	glUniform1f(effects.star_box.bpos_size, BPOS_CELL_SIZE);
	glUniform1f(effects.star_box.log_depth_intermediate_factor, log_depth_intermediate_factor);
	glBindVertexArray(0);
}

void star_box_deinit()
{
	glDeleteVertexArrays(NUM_BOXES, star_box.vaos);
	glDeleteBuffers(NUM_BOXES, star_box.vbos);
}

void star_box_draw(bpos_origin camera_origin, float proj_view_mat[16])
{
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glEnable(GL_BLEND); //We're going to blend the contribution from each individual star.
	glBlendEquation(GL_FUNC_ADD); //The light contributions get blended additively.
	glBlendFunc(GL_ONE, GL_ONE); //Just straight addition.
	for (int i = 0; i < NUM_BOXES; i++) {
		glUseProgram(effects.star_box.handle);
		glBindVertexArray(star_box.vaos[i]);
		glUniformMatrix4fv(effects.star_box.model_view_projection_matrix, 1, GL_TRUE, proj_view_mat);
		vec3 eye_box_offset = bpos_disp(camera_origin, star_box.origins[i]);
		float e[] = {eye_box_offset.x, eye_box_offset.y, eye_box_offset.z};
		glUniform3fv(effects.star_box.eye_box_offset, 1, e);
		// glUniform3fv(effects.star_box.eye_box_offset, 1, (float *)&eye_box_offset);
		glDrawArrays(GL_POINTS, 0, STARS_PER_BOX);
	}
	glDisable(GL_BLEND);
}

// star_box_nearest_star(bpos_origin pos)
// {

// }

scriptable_callback(star_box_script)
{
	Physical *camera = ((Entity *)entity->scriptable->context)->physical;
	star_box_update(camera->origin);
}