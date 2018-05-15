#ifndef SCRIPTABLE_H
#define SCRIPTABLE_H
#include "entity.h"

#define scriptable_callback(name) void name(Entity *entity)
typedef scriptable_callback(scriptable_callback_fn);

typedef struct scriptable_component {
	scriptable_callback_fn *script;
	void *context;
} Scriptable;

scriptable_callback(noop_script);
scriptable_callback(camera_script);
scriptable_callback(sun_script); // So I can play with the sun brightness

#endif