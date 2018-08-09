#ifndef STAR_BOX_H
#define STAR_BOX_H
#include "math/bpos.h"
#include "entity/scriptable.h"

void star_box_update(bpos_origin observer);
void star_box_draw(bpos_origin camera_origin, float proj_view_mat[16]);
void star_box_init(bpos_origin observer);
void star_box_deinit();
scriptable_callback(star_box_script);

int star_box_find_nearest_star_idx(bpos_origin pt, double *dist);
qvec3 star_box_get_star_origin(int star_idx);
int star_box_idx_from_star_idx(int star_idx);

#endif