#ifndef CONTROLLABLE_H
#define CONTROLLABLE_H

#include "entity.h"
#include "../input_event.h"

#define controllable_callback(name) void name(struct controller_axis_input input, bool buttons[16], struct mouse_input mouse, Entity *entity)
typedef controllable_callback(controllable_callback_fn);

typedef struct controllable_component {
	controllable_callback_fn *control;
	void *context;
} Controllable;

controllable_callback(noop_control);
controllable_callback(ship_control);
controllable_callback(camera_control);

#endif