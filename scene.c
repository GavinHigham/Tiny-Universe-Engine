#include "scene.h"
#include <stdbool.h>
#include <stdio.h>
#include <signal.h>

//It might be fun to have a simple default scene in these later.
SCENE_IMPLEMENT(empty);
int empty_scene_init(bool reload)
{
	if (reload)
		printf("Empty scene reloaded.\n");
	else
		printf("Empty scene selected.\n");
	return 0;
}
void empty_scene_deinit() {}
void empty_scene_update(float dt) {}
void empty_scene_render() {}
void empty_scene_resize(float width, float height) {}

int scene_error(char *fn_name, int error);
#define SAFE_CALL(fn, ...) (fn ? fn(__VA_ARGS__) : scene_error(#fn, -2))

struct game_scene current_scene = {.deinit = empty_scene_deinit}, *pcurrent_scene = NULL;
struct game_scene *next_scene = &empty_scene;
static struct game_scene *error_scene = NULL;
float scene_width = 800, scene_height = 600;

static void scene_swap()
{
	if (next_scene) {
		SAFE_CALL(current_scene.deinit);
		//pcurrent_scene is where the contents of current_scene came from
		if (next_scene != &current_scene)
			pcurrent_scene = next_scene;
		int error = SAFE_CALL(next_scene->init, next_scene->reloaded);
		next_scene->reloaded = true;
		if (error != 0 && error != -2) {
			SAFE_CALL(next_scene->deinit);
			scene_error("current_scene.init", error);
		} else {
			current_scene = *next_scene;
			//If we successfully got out of the empty scene, clear the error scene
			if (next_scene == error_scene)
				error_scene = NULL;
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
	//If we're in the empty scene because a scene hit an error, and there is no next_scene,
	//retry the errored scene (the error may have been fixed and this reload is testing the fix)
	if (pcurrent_scene == &empty_scene && error_scene && !next_scene) {
		scene_set(error_scene);
	} else {
		scene_set(&current_scene);
	}
}

void scene_filedrop(const char *file)
{
	scene_swap();
	//filedrop is optional and rare, don't error if it's not present.
	if (current_scene.filedrop)
		current_scene.filedrop(file);
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
	if (pcurrent_scene != &empty_scene)
		error_scene = pcurrent_scene;
	scene_set(NULL);
	scene_swap();
	return error;
}