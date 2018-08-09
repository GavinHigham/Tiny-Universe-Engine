#ifndef SPACE_SCENE_H
#define SPACE_SCENE_H
#include <GL/glew.h>
#include "scene.h"

extern float screen_width;
extern float screen_height;
extern struct game_scene space_scene;
int space_scene_init();
void space_scene_deinit();
void space_scene_update(float dt); //should be moved to its own source file when things get big
void space_scene_render();

void space_scene_queue_reload();
void space_scene_resize(float width, float height);
void make_projection_matrix(GLfloat fov, GLfloat a, GLfloat n, GLfloat f, GLfloat *buf);

#endif