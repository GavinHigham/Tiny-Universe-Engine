#include <stdlib.h>
#include <stdbool.h>
#include "drawable.h"
#include "glla.h"
#include "effects.h"
#include "math/bpos.h"
#include "buffer_group.h"
#include "entity/entity.h"

/*
Gavin Update notes:
The drawable system should be made to collaborate with the ECS
*/

extern bpos_origin eye_sector;

void init_drawable(Drawable *d, void (*draw)(EFFECT *, struct buffer_group, amat4), EFFECT *effect, amat4 *frame, bpos_origin *sector, struct buffer_group *buffers)
{
	d->effect = effect;
	d->frame = frame;
	d->sector = sector;
	d->bg = buffers;
	d->draw = draw;
	d->bg_from_malloc = false;
}

void init_heap_drawable(Drawable *d, Draw_func draw, EFFECT *effect, amat4 *frame, bpos_origin *sector, int (*buffering_function)(struct buffer_group))
{
	struct buffer_group *bg = malloc(sizeof(struct buffer_group));
	*bg = new_buffer_group(buffering_function, effect);
	init_drawable(d, draw, effect, frame, sector, bg);
	d->bg_from_malloc = true;
}

void deinit_drawable(Drawable *d)
{
	if (d->bg_from_malloc)
		free(d->bg);
}

void draw_drawable(Drawable *d)
{
	d->draw(d->effect, *d->bg, (amat4){d->frame->a, bpos_remap((bpos){d->frame->t, *(d->sector)}, eye_sector)});
}

void entity_make_drawable(Entity *e, Drawable d)
{
	entity_alloc_drawable(e, d);
}

void entity_unmake_drawable(Entity *e)
{
	entity_dealloc_drawable(e);
}