#include "star_box.h"
#include "glla.h"
#include "math/bpos.h"
#include "math/utility.h"
#include "graphics.h"
#include "entity/scriptable.h"
#include "entity/physical.h"
#include "macros.h"
#include "open-simplex-noise-in-c/open-simplex-noise.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <stdbool.h>

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

//HALF_BOX_SIZE and -HALF_BOX_SIZE need to be representable in an int32_t.
#define STAR_BOX_SIZE 4294967296 //67108864 //8096 //The width of one edge of a star box, in bpos cell widths.
#define STAR_SIZE (sizeof(int32_t) * 3)
#define HALF_BOX_SIZE (STAR_BOX_SIZE / 2)

static bpos_origin star_box_round_pt(bpos_origin o, int64_t precision)
{
	//Shift o by half a box width, and mask off some low-order bits.
	//Will this cause endian-ness errors? (Do I care?)
	return (o + precision / 2) & ~(precision-1);
}

/*
let b = STAR_BOX_BUCKET_DIVS_PER_AXIS
there are b^3 buckets, meaning indexes go from [0, b^3)
let s = STAR_BOX_SIZE and h = HALF_BOX_SIZE
a star at {h, h, h} should be in bucket b^3-1
a star at {-h, -h, -h} should be in bucket 0


*/
uint32_t bucket_idx(int32_t star[3])
{
	uint32_t idx = 0;
	for (int i = 0, d = 1; i < 3; i++, d *= STAR_BOX_BUCKET_DIVS_PER_AXIS)
		idx += (star[i] + HALF_BOX_SIZE) * d / STAR_BOX_SIZE;
	return idx;
}

uint32_t star_box_idx_from_star_idx(uint32_t star_idx)
{
	//TODO(Gavin) Check this for errors.
	return star_idx / STAR_BOX_STARS_PER_BOX;
}

static void star_box_generate(struct star_box_ctx *sb, bpos_origin o, uint32_t box)
{
	vec3 c1 = -HALF_BOX_SIZE;
	vec3 c2 =  HALF_BOX_SIZE;
	uint32_t hash = hash_qvec3(o);
	srand(hash);
	srand_float(hash);
	int32_t *stars = &sb->stars[STAR_BOX_STARS_PER_BOX * 3 * box];
	uint32_t *buckets = &sb->start_indices[STAR_BOX_BUCKETS_PER_BOX * box];
	//The number of stars in each bucket
	uint32_t bucket_counts[STAR_BOX_BUCKETS_PER_BOX] = {0};

	for (uint32_t i = 0; i < STAR_BOX_STARS_PER_BOX; i++) {
		vec3 v = rand_box_vec3(c1, c2);
		int32_t *star = &stars[3*i];
		star[0] = v.x; star[1] = v.y; star[2] = v.z;
		//memcpy(star, &v, STAR_SIZE);
		//Count the number of stars that will go into each bucket.
		bucket_counts[bucket_idx(star)]++;
	}

	//Each bucket stores the start index of the stars within it.
	uint32_t start = 0;
	for (uint32_t i = 0; i < STAR_BOX_BUCKETS_PER_BOX; i++) {
		buckets[i] = start;
		start += bucket_counts[i];
	}

	#define FIRST_UNSORTED_IDX(bucket_idx) (buckets[bucket_idx+1] - bucket_counts[bucket_idx])
	for (uint32_t i = 0; i < STAR_BOX_BUCKETS_PER_BOX; i++) {
		while (bucket_counts[i] > 0) {
			int32_t *star = &stars[FIRST_UNSORTED_IDX(i) * 3];
			uint32_t sbi = bucket_idx(star);
			int32_t *dest = &stars[FIRST_UNSORTED_IDX(sbi) * 3];

			//Swap first unsorted in current bucket with first unsorted from destination bucket.
			int32_t tmp_star[3];
			memcpy(tmp_star, dest, STAR_SIZE);
			memcpy(dest, star, STAR_SIZE);
			memcpy(star, tmp_star, STAR_SIZE);
			bucket_counts[sbi]--;
		}
	}

	//Validate buckets
	// for (int i = 0; i < STAR_BOX_BUCKETS_PER_BOX; i++) {
	// 	for (int j = buckets[i]; j < buckets[i+1]; j++) {
	// 		if (bucket_idx(&stars[j*3]) != i)
	// 			printf("Validating bucket %i [%i, %i), star %i should be in bucket %i.\n", i, buckets[i], buckets[i+1], j, bucket_idx(&stars[j*3]));
	// 		//assert(bucket_idx(&stars[j*3]) == i);
	// 	}
	// }
}

static uint32_t idx_from_pt(bpos_origin pt)
{
	qvec3 idx;
	for (int i = 0; i < 3; i++) {
		idx[i] = lldiv_floor(pt[i], STAR_BOX_SIZE).quot % 3;
		idx[i] += 3*(idx[i] < 0);
	}
	return idx.x + idx.y * 3 + idx.z * 9;
}

static void bucket_from_pt(struct star_box_ctx *sb, bpos_origin pt, uint32_t *box, uint32_t *bucket)
{
	*box = idx_from_pt(star_box_round_pt(pt, STAR_BOX_SIZE));
	qvec3 tmp = pt - sb->origins[*box]; //pt relative to box origin.
	*bucket = bucket_idx((int32_t[3]){VEC3_COORDS(tmp)});
}

static void nearest_star_in_bucket(struct star_box_ctx *sb, uint32_t box, uint32_t bucket, qvec3 pt, uint32_t *star_idx, double *closest_dist)
{
	int32_t *stars = &sb->stars[STAR_BOX_STARS_PER_BOX * 3 * box];
	uint32_t *buckets = &sb->start_indices[STAR_BOX_BUCKETS_PER_BOX * box];
	for (uint32_t i = buckets[bucket]; i < buckets[bucket+1]; i++) {
		qvec3 star = {stars[i*3], stars[i*3 + 1], stars[i*3 + 2]};
		double star_dist = qvec3_distd(pt - sb->origins[box], star);

		if (star_dist < *closest_dist) {
			*closest_dist = star_dist;
			*star_idx = i;
		}
	}
}

//Make this slow and branchy at first, can optimize later.
uint32_t star_box_find_nearest_star_idx(struct star_box_ctx *sb, bpos_origin pt, double *dist)
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
	For each edge of b, e, let m be the point on e closest to p, and let d = (m - p)/|m - p|.
		Let p' = |a|d + p. If p' lies in a different bucket, search that bucket.

	Alternatively, I could just iterate through all 26 neighbor buckets and check every star.
	*/

	uint32_t box, bucket, spill_box, spill_bucket, star_idx = -1;
	double closest_dist = INFINITY;
	double bucket_width = sqrt(2) * (STAR_BOX_SIZE/STAR_BOX_BUCKET_DIVS_PER_AXIS);
	bucket_from_pt(sb, pt, &box, &bucket);
	nearest_star_in_bucket(sb, box, bucket, pt, &star_idx, &closest_dist);

	//Vector from the center towards the 6 faces of a bucket cube.
	qvec3 faces[] = {
		{0, 0, -1}, {0, 0, 1}, {0, -1, 0}, {0, 1, 0}, {-1, 0, 0}, {1, 0, 0}
	};

	//Edges defined by indices of the two faces they are shared by.
	uint32_t edges[12][2] = {
		{0, 2}, {0, 3}, {0, 4}, {0, 5},
		{1, 2}, {1, 3}, {1, 4}, {1, 5},
		{3, 4}, {3, 5}, {2, 4}, {2, 5}
	};

	//Vertices defined by indices of the three faces they are shared by.
	uint32_t vertices[8][3] = {
		{0, 2, 4}, {0, 2, 5}, {0, 3, 4}, {0, 3, 5},
		{1, 2, 4}, {1, 2, 5}, {1, 3, 4}, {1, 3, 5}
	};

	bool face_intersected[6] = {0};

	for (uint32_t i = 0; i < LENGTH(faces); i++) {
		//If we can shave a little distance off, we can avoid searching the bucket.
		qvec3 offset = (closest_dist == INFINITY) ?
			faces[i] * (int64_t)bucket_width :
			faces[i] * (int64_t)closest_dist;
		bucket_from_pt(sb, pt + offset, &spill_box, &spill_bucket);
		if (spill_bucket == bucket && spill_box == box)
			continue;

		face_intersected[i] = true;
		nearest_star_in_bucket(sb, spill_box, spill_bucket, pt, &star_idx, &closest_dist);
	}

	for (int i = 0; i < LENGTH(edges); i++) {
		if (!face_intersected[edges[i][0]] || !face_intersected[edges[i][1]])
			continue;

		//Sphere surrounding pt definitely intersects this bucket, just search it.
		qvec3 offset = (faces[edges[i][0]] + faces[edges[i][1]]) * (int64_t)bucket_width;
		bucket_from_pt(sb, pt + offset, &spill_box, &spill_bucket);
		nearest_star_in_bucket(sb, spill_box, spill_bucket, pt, &star_idx, &closest_dist);
	}

	for (int i = 0; i < LENGTH(vertices); i++) {
		if (!face_intersected[vertices[i][0]] || !face_intersected[vertices[i][1]] || !face_intersected[vertices[i][2]])
			continue;

		//Sphere surrounding pt definitely intersects this bucket, just search it.
		qvec3 offset = (faces[vertices[i][0]] + faces[vertices[i][1]] + faces[vertices[i][2]]) * (int64_t)bucket_width;
		bucket_from_pt(sb, pt + offset, &spill_box, &spill_bucket);
		nearest_star_in_bucket(sb, spill_box, spill_bucket, pt, &star_idx, &closest_dist);
	}

	if (dist)
		*dist = closest_dist;

	return star_idx;
}

qvec3 star_box_get_star_origin(struct star_box_ctx *sb, uint32_t star_idx)
{
	uint32_t box = star_box_idx_from_star_idx(star_idx);
	int32_t *star = &sb->stars[star_idx * 3];
	return sb->origins[box] + (qvec3){star[0], star[1], star[2]};
}

/*
Find the star box for the observer position.
For each box in 3x3x3 about that box, check that the origin matches.
Generate any box that does not have a matching origin.
*/

void star_box_update(struct star_box_ctx *sb, bpos_origin observer)
{
	bpos_origin center = star_box_round_pt(observer, STAR_BOX_SIZE);
	bool any_changed = false;

	for (uint32_t i = 0; i < 3; i++) {
		for (uint32_t j = 0; j < 3; j++) {
			for (uint32_t k = 0; k < 3; k++) {
				//vector index of the box, an integer along each axis of the box continuum
				qvec3 box_idx = qhypertoroidal_buffer_slot(observer, STAR_BOX_SIZE, 3, (qvec3){i,j,k});
				bpos_origin new_origin = box_idx * STAR_BOX_SIZE;
				uint32_t idx = i + j * 3 + k * 9;
				if (memcmp(&new_origin, &sb->origins[idx], sizeof(new_origin))) {
					any_changed = true;
					printf("Replacing box at %i, new origin ", idx); qvec3_println(new_origin);
					//Generate the stars for that star box.
					sb->origins[idx] = new_origin;
					star_box_generate(sb, box_idx, idx);
					glBindBuffer(GL_ARRAY_BUFFER, sb->vbos[idx]);
					glBufferData(GL_ARRAY_BUFFER, STAR_SIZE*STAR_BOX_STARS_PER_BOX, &sb->stars[idx*3*STAR_BOX_STARS_PER_BOX], GL_STATIC_DRAW);
				}
			}
		}
	}

	if (any_changed) {
		printf("Updating stars, new center at "); qvec3_print(center); puts("");
	}
}

struct star_box_ctx * star_box_new()
{
	return malloc(sizeof(struct star_box_ctx));
}

struct star_box_ctx * star_box_init(struct star_box_ctx *sb, bpos_origin observer)
{
	srand(101);
	glGenVertexArrays(STAR_BOX_NUM_BOXES, sb->vaos);
	glGenBuffers(STAR_BOX_NUM_BOXES, sb->vbos);
	for (int i = 0; i < STAR_BOX_NUM_BOXES; i++) {
		glBindVertexArray(sb->vaos[i]);
		glEnableVertexAttribArray(effects.star_box.star_pos);
		glBindBuffer(GL_ARRAY_BUFFER, sb->vbos[i]);
		glVertexAttribPointer(effects.star_box.star_pos, 3, GL_INT, GL_FALSE, 0, NULL);
		sb->origins[i] = observer + 42; //Subtle bug: An initial 0,0,0 origin would not be generated.
	}
	star_box_update(sb, observer);
	glUseProgram(effects.star_box.handle);
	glUniform1f(effects.star_box.star_box_size, STAR_BOX_SIZE);
	glUniform1f(effects.star_box.bpos_size, BPOS_CELL_SIZE);
	// glUniform1f(effects.star_box.log_depth_intermediate_factor, log_depth_intermediate_factor);
	glBindVertexArray(0);
	return sb;
}

void star_box_deinit(struct star_box_ctx *sb)
{
	glDeleteVertexArrays(STAR_BOX_NUM_BOXES, sb->vaos);
	glDeleteBuffers(STAR_BOX_NUM_BOXES, sb->vbos);
}

void star_box_draw(struct star_box_ctx *sb, bpos_origin camera_origin, float proj_view_mat[16])
{
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glEnable(GL_BLEND); //We're going to blend the contribution from each individual star.
	glBlendEquation(GL_FUNC_ADD); //The light contributions get blended additively.
	glBlendFunc(GL_ONE, GL_ONE); //Just straight addition.
	for (int i = 0; i < STAR_BOX_NUM_BOXES; i++) {
		glUseProgram(effects.star_box.handle);
		glBindVertexArray(sb->vaos[i]);
		glUniformMatrix4fv(effects.star_box.model_view_projection_matrix, 1, GL_TRUE, proj_view_mat);
		vec3 eye_box_offset = bpos_disp(camera_origin, sb->origins[i]);
		float e[] = {eye_box_offset.x, eye_box_offset.y, eye_box_offset.z};
		glUniform3fv(effects.star_box.eye_box_offset, 1, e);
		// glUniform3fv(effects.sb->eye_box_offset, 1, (float *)&eye_box_offset);
		glDrawArrays(GL_POINTS, 0, STAR_BOX_STARS_PER_BOX);
	}
	glDisable(GL_BLEND);
}

// star_box_nearest_star(bpos_origin pos)
// {

// }