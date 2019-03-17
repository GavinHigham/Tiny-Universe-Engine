#ifndef SPIRAL_SCENE_H
#define SPIRAL_SCENE_H

#include "glla.h"
#include "graphics.h"
#include "deferred_framebuffer.h"
#include "configuration/lua_configuration.h"
#include "meter/meter.h"
#include <stdbool.h>
extern struct game_scene spiral_scene;
extern meter_ctx g_galaxy_meters;

int spiral_scene_init();
void spiral_scene_deinit();
void spiral_scene_resize(float width, float height);
void spiral_scene_update(float dt);

#endif