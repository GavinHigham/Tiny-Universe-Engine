#include "entity/breadcrumber.h"
#include "entity/physical.h"
#include "math/bpos.h"
#include <stdbool.h>
#include <stdlib.h>

int circular_buffer_next(int start, int num, int max)
{
	return (start + num) % max;
}

int circular_buffer_last(int start, int num, int max)
{
	return (start + num - 1) % max;
}

void breadcrumber_try_crumb(Entity *entity)
{
	Breadcrumber *bc = entity->breadcrumber;
	bc->crumb_left_last_update = false;
	bpos *last_crumb = &bc->crumbs[circular_buffer_last(bc->start, bc->num, bc->max)];
	bpos new_crumb = {entity->physical->position.t, entity->physical->origin};
	if (bc->num == 0 || bpos_distd(new_crumb, *last_crumb) > bc->threshold) {
		bc->crumbs[circular_buffer_next(bc->start, bc->num, bc->max)] = new_crumb;
		bc->crumb_left_last_update = true;
	}
}

void entity_make_breadcrumber(Entity *e, Breadcrumber bc)
{
	bc.crumbs = malloc(sizeof(bpos) * bc.max);
	entity_alloc_breadcrumber(e, bc);
}

void entity_unmake_breadcrumber(Entity *e)
{
	free(e->breadcrumber->crumbs);
	entity_dealloc_breadcrumber(e);
}