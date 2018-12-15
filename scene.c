#include "scene.h"
#include <stdbool.h>
#include <stdio.h>
#include <signal.h>

//It might be fun to have a simple default scene in these later.
SCENE_IMPLEMENT(empty);
int empty_scene_init() { printf("Empty scene selected.\n"); return 0;}
void empty_scene_deinit() {}
void empty_scene_update(float dt) {}
void empty_scene_render() {}
void empty_scene_resize(float width, float height) {}

int scene_error(char *fn_name, int error);
#define SAFE_CALL(fn, ...) (fn ? fn(__VA_ARGS__) : scene_error(#fn, -2))

struct game_scene current_scene = {.deinit = empty_scene_deinit};
struct game_scene *next_scene = &empty_scene;
float scene_width = 800, scene_height = 600;

static void scene_swap()
{
	if (next_scene) {
		SAFE_CALL(current_scene.deinit);
		int error = SAFE_CALL(next_scene->init);
		if (error != 0 && error != -2) {
			scene_error("current_scene.init", error);
		} else {
			current_scene = *next_scene;
			next_scene = NULL;
			SAFE_CALL(current_scene.resize, scene_width, scene_height);
		}
	}
}

void scene_set(struct game_scene *scene)
{
	next_scene = scene ? scene : &empty_scene;
}

void scene_update(float dt)
{
	scene_swap();
	SAFE_CALL(current_scene.update, dt);
}

void scene_render()
{
	scene_swap();
	SAFE_CALL(current_scene.render);
}

void scene_resize(float width, float height)
{
	scene_swap();
	scene_width = width;
	scene_height = height;
	SAFE_CALL(current_scene.resize, width, height);
}

void scene_reload()
{
	scene_set(&current_scene);
}

int scene_error(char *fn_name, int error)
{
	switch (error) {
	case 0: return error;
	case -2:
		printf("Scene error: %s was NULL\n", fn_name);
		break;
	default:
		printf("Scene SAFE_CALL of %s failed with error %i\n", fn_name, error);
		break;
	}
	scene_set(NULL);
	scene_swap();
	return error;
}