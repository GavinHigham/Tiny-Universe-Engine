#ifndef DRAW_H
#define DRAW_H

#include "glla.h"
#include "graphics.h"
#include "buffer_group.h"

void draw_forward(EFFECT *effect, struct buffer_group bg, amat4 model_matrix);
void draw_wireframe(EFFECT *effect, struct buffer_group bg, amat4 model_matrix);
void draw_forward_adjacent(EFFECT *effect, struct buffer_group bg, amat4 model_matrix);
void draw_skybox_forward(EFFECT *e, struct buffer_group bg, amat4 model_matrix);

#endif