#ifndef COMPONENTS_H
#define COMPONENTS_H

#include "datastructures/ecs.h"
#include "glla.h"
#include "math/bpos.h"

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

typedef struct component_controllable {
	// controllable_callback_fn *control;
	void *context;
} ControllableTemp;

typedef struct component_customdrawable {
	void (*draw)(ecs_ctx *E, uint32_t camera, uint32_t self, void *ctx);
	void *ctx;
} CustomDrawable;

typedef struct component_label {
	char *name;
	char *description;
} Label;

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

#define scriptable_callback(name) void name(uint32_t *entity)
typedef scriptable_callback(scriptable_callback_fn);

typedef struct component_scriptable {
	scriptable_callback_fn *script;
	void *context;
} ScriptableTemp;

typedef struct component_universal {

} Universal;

#endif