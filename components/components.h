
#ifndef COMPONENTS_H
#define COMPONENTS_H

#include "datastructures/ecs.h"
#include "glla.h"
#include "math/bpos.h"
#include "graphics.h"
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
	float proj_view_mat[16]; //Will be updated once per frame
	float width, height;
	hframebuffer fbo; //Framebuffer object
	// Camera will '&' drawable's draw_group with this bitfield and draw if the result is nonzero,
	// unless draw_groups is 0, in which case the camera will draw any drawable, regardless of draw group.
	// Essentially, draw if (drawable.draw_group & camera.draw_groups) == camera.draw_groups
	uint32_t draw_groups;
	//Used to draw cameras "in order". Setting "next" to a camera entity that depends on this one will form a DAG,
	//cameras will be rendered starting with those that have no prerequisites, until all prerequisites are satisfied.
	uint32_t next; //Entity (with camera component) that depends on this one.
	size_t num_prev; //Number of predecessor cameras that this one depends on (will be set and reset during rendering)
	bool visited; //True if this camera has been visited already, when finding the topological sort of the graph.
} Camera;
CD(camera_constructor);

typedef struct component_controllable {
	// controllable_callback_fn *control;
	void *context;
} ControllableTemp; //TODO(Gavin): Rename "Controllable" when universe_scene reaches parity with space_scene

#define customdrawable_callback(name) void name(ecs_ctx *E, uint32_t camera, uint32_t self, void *ctx)
typedef customdrawable_callback(customdrawable_callback_fn);
typedef struct component_customdrawable {
	customdrawable_callback_fn *draw;
	//"Custom" constructor/destructor for each entity that has this component.
	ecs_c_constructor_fn *construct;
	ecs_c_destructor_fn *destruct;
	void *ctx;
	uint32_t draw_group;
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
	void *ctx;
} ScriptableTemp;

typedef struct component_universal {

} Universal;

typedef struct component_plymesh {
	struct ply_mesh *mesh;
} PlyMesh;

typedef struct component_target {
	uint32_t target;
} Target;

#endif