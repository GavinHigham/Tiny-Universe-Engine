#include "experiments/universe_scene/universe_components.h"
#include "experiments/universe_scene/universe_ecs.h"
#include "trackball/trackball.h"
#include <assert.h>

CD(trackball_constructor)
{
	Trackball *t = component;
	PhysicalTemp *self = entity_physicaltemp(eid);
	PhysicalTemp *target = entity_physicaltemp(t->target);
	assert(self);
	assert(target);
	//In case the target has a different origin. Will still need special handling when updating.
	double distance = bpos_distd((bpos){self->position.t, self->origin}, (bpos){target->position.t, target->origin});
	t->trackball = trackball_new(target->position.t, distance);
	trackball_set_speed(&t->trackball, .01, .01, .01);
}