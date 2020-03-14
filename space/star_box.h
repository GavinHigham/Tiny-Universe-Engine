#ifndef STAR_BOX_H
#define STAR_BOX_H
#include "math/bpos.h"
#include "entity/scriptable.h"

enum {
	//TODO: Consider if I can just keep this small and multiply in the vertex shader.
	STAR_BOX_STARS_PER_BOX = 10000, //The number of stars per "star box", of which there are STAR_BOX_NUM_BOXES.
	STAR_BOX_NUM_BOXES = 27,
	STAR_BOX_BUCKET_DIVS_PER_AXIS = 16,
	STAR_BOX_BUCKETS_PER_BOX = STAR_BOX_BUCKET_DIVS_PER_AXIS*STAR_BOX_BUCKET_DIVS_PER_AXIS*STAR_BOX_BUCKET_DIVS_PER_AXIS
};

struct star_box_ctx {
	uint32_t       vaos[STAR_BOX_NUM_BOXES];
	uint32_t       vbos[STAR_BOX_NUM_BOXES];
	bpos_origin origins[STAR_BOX_NUM_BOXES];
	int32_t       stars[STAR_BOX_NUM_BOXES*3*STAR_BOX_STARS_PER_BOX];
	uint32_t start_indices[STAR_BOX_NUM_BOXES * STAR_BOX_BUCKETS_PER_BOX];
};

void star_box_update(struct star_box_ctx *sb, bpos_origin observer);
void star_box_draw(struct star_box_ctx *sb, bpos_origin camera_origin, float proj_view_mat[16]);
struct star_box_ctx * star_box_new();
void star_box_free(struct star_box_ctx *sb);
struct star_box_ctx * star_box_init(struct star_box_ctx *sb, bpos_origin observer);
void star_box_deinit(struct star_box_ctx *sb);

uint32_t star_box_find_nearest_star_idx(struct star_box_ctx *sb, bpos_origin pt, double *dist);
qvec3 star_box_get_star_origin(struct star_box_ctx *sb, uint32_t star_idx);
uint32_t star_box_idx_from_star_idx(uint32_t star_idx);

#endif