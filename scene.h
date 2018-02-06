#ifndef SCENE_H
#define SCENE_H

typedef int (scene_init_fn)(void);
typedef void (scene_deinit_fn)(void);
typedef void (scene_update_fn)(float dt);
typedef void (scene_render_fn)(void);
typedef void (scene_resize_fn)(float width, float height);

struct game_scene {
	scene_init_fn *init;
	scene_deinit_fn *deinit;
	scene_update_fn *update;
	scene_render_fn *render;
	scene_resize_fn *resize;
};

void scene_set(struct game_scene *scene);
void scene_update(float dt);
void scene_render();
void scene_resize(float width, float height);
void scene_reload();

#define SCENE_FUNCS(name)                                     \
	int  name##_scene_init();                                 \
	void name##_scene_deinit();                               \
	void name##_scene_update(float dt);                       \
	void name##_scene_render();                               \
	void name##_scene_resize(float width, float height);
#define SCENE_VTABLE(name) struct game_scene name##_scene = { \
	name##_scene_init,                                        \
	name##_scene_deinit,                                      \
	name##_scene_update,                                      \
	name##_scene_render,                                      \
	name##_scene_resize                                       \
};
#define SCENE_IMPLEMENT(name)                                 \
	SCENE_FUNCS(name)                                         \
	SCENE_VTABLE(name)

#endif