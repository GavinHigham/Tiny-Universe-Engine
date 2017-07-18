#ifndef STAR_BLOCKS_H
#define STAR_BLOCKS_H
#include "math/bpos.h"
#include "entity/scriptable.h"

void star_blocks_update(bpos_origin observer);
void star_blocks_draw(bpos_origin camera_origin);
void star_blocks_init(bpos_origin observer);
void star_blocks_deinit();
scriptable_callback(star_blocks_script);

#endif