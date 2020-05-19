
#ifndef COMPONENTS_H
#define COMPONENTS_H

#include "datastructures/ecs.h"
#include "glla.h"
#include "math/bpos.h"
#define CD ecs_c_constructor_destructor

//Possible future work: Parse this file (or parse a separate schema file to generate this)
//and generate Lua getters/setters.

typedef struct component_camera {
	float fov; //Field of view in radians
	float aspect; //Aspect ratio, width/height.
	float near, far; //Distance to near and far planes.
	float log_depth_intermediate_factor; //Intermediate factor used when rendering with logarithmic depth.
	bool log_depth;
	float proj_mat[16];
} Camera;
CD(camera_constructor);

typedef struct component_framebuffer {
	//TODO(Gavin): Figure out what triggers a framebuffer to be rendered. An event? A script?
	//Need "every frame" for some, and "only when needed" for others.
	//I don't like the idea of making the ECS aware of OpenGL, but this is easy enough for now.
	//Later would be nice to have abstract handles to color buffers, depth buffer, etc.
	//Basically just want an abstract interface that OpenGL conforms to, so I can change the backend later.
	//sokol_gfx?
	union {
		uint32_t ogl_fbo;
	};
	bool render_every_frame;
	uint32_t camera; //Camera that will render to this framebuffer.
	float width, height;
} Framebuffer;

typedef struct component_controllable {
	// controllable_callback_fn *control;
	void *context;
} ControllableTemp;

#define customdrawable_callback(name) void name(ecs_ctx *E, uint32_t camera, uint32_t self, void *ctx)
typedef customdrawable_callback(customdrawable_callback_fn);
typedef struct component_customdrawable {
	customdrawable_callback_fn *draw;
	//"Custom" constructor/destructor for each entity that has this component.
	ecs_c_constructor_fn *construct;
	ecs_c_destructor_fn *destruct;
	void *ctx;
} CustomDrawable;
CD(customdrawable_constructor);
CD(customdrawable_destructor);

//TODO(Gavin): Give label a GC / generic constructor specialization
typedef struct component_label {
	char *name;
	char *description;
} Label;
CD(label_constructor);
CD(label_destructor);

//Temporary copy of "Physical" component while I refactor
typedef struct component_physicaltemp {
	float bounding_sphere; //Stored as radius squared, centered on position.t
	float mass;
	mat3 moment;
	amat4 position;
	amat4 velocity;
	amat4 acceleration;
	bpos_origin origin; //May later want to move this into a separate component for performance.
} PhysicalTemp;

#define scriptabletemp_callback(name) void name(uint32_t eid)
typedef scriptabletemp_callback(scriptabletemp_callback_fn);
typedef struct component_scriptable {
	scriptabletemp_callback_fn *script;
	void *context;
} ScriptableTemp;

typedef struct component_universal {

} Universal;

typedef struct component_target {
	uint32_t target;
} Target;

#endif