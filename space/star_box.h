#ifndef STAR_BOX_H
#define STAR_BOX_H
#include "../math/bpos.h"
#include "../entity/scriptable.h"

void star_box_update(bpos_origin observer);
void star_box_draw(bpos_origin camera_origin, float proj_view_mat[16]);
void star_box_init(bpos_origin observer);
void star_box_deinit();
scriptable_callback(star_box_script);

#endif