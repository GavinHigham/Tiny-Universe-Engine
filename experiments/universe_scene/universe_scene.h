#ifndef UNIVERSE_SCENE_H
#define UNIVERSE_SCENE_H

#include "scene.h"

extern struct game_scene universe_scene;
int universe_scene_init();
void universe_scene_deinit();
void universe_scene_update(float dt);
void universe_scene_render();
void universe_scene_resize(float width, float height);

#endif