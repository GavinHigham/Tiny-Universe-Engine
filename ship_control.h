#ifndef SHIP_CONTROL_H
#define SHIP_CONTROL_H
#include "glla.h"
#include "math/space_sector.h"

struct controller_input {
	float leftx;
	float lefty;
	float rightx;
	float righty;
	float ltrigger;
	float rtrigger;
};

struct ship_physics {
	//In the future, use quarternions for rotation.
	float speed;
	float mass;
	vec3 acceleration;
	amat4 velocity;
	amat4 position;
	amat4 locked_camera;
	amat4 eased_camera;
	vec3 locked_camera_target;
	vec3 eased_camera_target;
	vec3 movable_camera;
	space_sector sector;
};

struct ship_physics ship_control(float dt, struct controller_input input, struct ship_physics ship);

#endif