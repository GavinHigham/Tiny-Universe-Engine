#ifndef SCENE_H
#define SCENE_H

#include <stdbool.h>

typedef int (scene_init_fn)(bool reload);
typedef void (scene_deinit_fn)(void);
typedef void (scene_update_fn)(float dt);
typedef void (scene_render_fn)(void);
typedef void (scene_resize_fn)(float width, float height);
typedef void (scene_filedrop_fn)(const char *file);

struct game_scene {
	const char *name;
	bool reloaded;
	scene_init_fn *init;
	scene_deinit_fn *deinit;
	scene_update_fn *update;
	scene_render_fn *render;
	scene_resize_fn *resize;
	scene_filedrop_fn *filedrop;
};

void scene_set(struct game_scene *scene);
void scene_update(float dt);
void scene_render();
void scene_resize(float width, float height);
void scene_reload();
void scene_filedrop(const char *file);

//To implement the optional filedrop function,
//define SCENE_HAS_FILEDROP before including scene.h
#ifdef SCENE_HAS_FILEDROP
	#define FILEDROP_DECL(name) void name##_scene_filedrop(const char *file);
	#define FILEDROP_FNPTR(name) name##_scene_filedrop
#else
	#define FILEDROP_DECL(name)
	#define FILEDROP_FNPTR(name) NULL
#endif

#define SCENE_FUNCS(name)                                     \
	int  name##_scene_init(bool reload);                      \
	void name##_scene_deinit();                               \
	void name##_scene_update(float dt);                       \
	void name##_scene_render();                               \
	void name##_scene_resize(float width, float height);      \
	FILEDROP_DECL(name)

#define SCENE_VTABLE(name) struct game_scene name##_scene = { \
	#name,                                                    \
	false,													  \
	name##_scene_init,                                        \
	name##_scene_deinit,                                      \
	name##_scene_update,                                      \
	name##_scene_render,                                      \
	name##_scene_resize,                                      \
	FILEDROP_FNPTR(name)                                      \
};
#define SCENE_IMPLEMENT(name)                                 \
	SCENE_FUNCS(name)                                         \
	SCENE_VTABLE(name)

#endif