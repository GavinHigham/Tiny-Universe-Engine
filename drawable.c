#include <stdlib.h>
#include <stdbool.h>
#include "effects.h"
#include "buffer_group.h"
#include "drawable.h"
#include "glla.h"

void init_drawable(Drawable *d, void (*draw)(EFFECT *, struct buffer_group, amat4), EFFECT *effect, amat4 *frame, struct buffer_group *buffers)
{
	d->effect = effect;
	d->frame = frame;
	d->bg = buffers;
	d->draw = draw;
	d->bg_from_malloc = false;
}

void init_heap_drawable(Drawable *d, Draw_func draw, EFFECT *effect, amat4 *frame, int (*buffering_function)(struct buffer_group))
{
	struct buffer_group *bg = malloc(sizeof(struct buffer_group));
	*bg = new_buffer_group(buffering_function, effect);
	init_drawable(d, draw, effect, frame, bg);
	d->bg_from_malloc = true;
}

void deinit_drawable(Drawable *d)
{
	if (d->bg_from_malloc)
		free(d->bg);
}

void draw_drawable(Drawable *d)
{
	d->draw(d->effect, *d->bg, *d->frame);
}