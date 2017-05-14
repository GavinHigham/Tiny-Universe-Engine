#ifndef DRAWABLE_H
#define DRAWABLE_H

#include <stdbool.h>
#include "glla.h"
#include "../effects.h"
#include "../buffer_group.h"
#include "../math/bpos.h"

typedef struct drawable_component {
	EFFECT *effect;
	amat4 *frame;
	bpos_origin *sector;
	struct buffer_group *bg;
	void (*draw)(EFFECT *, struct buffer_group, amat4);
	bool bg_from_malloc; //Until I clean up buffer_group, check this to decide if free() should be called on bg within deinit.
} Drawable;

typedef void (*Draw_func)(EFFECT *, struct buffer_group, amat4);

void init_drawable(Drawable *d, void (*draw)(EFFECT *, struct buffer_group, amat4), EFFECT *effect, amat4 *frame, bpos_origin *sector, struct buffer_group *buffers);
//Temporary Drawable init until I clean up buffer_group stuff.
void init_heap_drawable(Drawable *d, Draw_func draw, EFFECT *effect, amat4 *frame, bpos_origin *sector, int (*buffering_function)(struct buffer_group));
void deinit_drawable(Drawable *drawable);
void draw_drawable(Drawable *d);

#endif