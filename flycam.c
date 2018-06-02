#include "entity/controllable.h"
#include "entity/physical.h"
#include "flycam.h"
#include "glla.h"

controllable_callback(flycam_control)
{
	Physical *camera = entity->physical;
	float camera_speed = 0.5;

	//Rotate camera by mouse.
	//Move WASD-wise in view-space.

	/*
	camera->position.t = camera->position.t + //Honestly I just tried things at random until it worked, but here's my guess:
		mat3_multvec(mat3_transp(ship->position.a), // 2) Convert those coordinates from world-space to ship-space.
			mat3_multvec(camera->position.a, (vec3){ // 1) Move relative to the frame pointed at the ship.
			(key_state[SDL_SCANCODE_D] - key_state[SDL_SCANCODE_A]) * camera_speed,
			(key_state[SDL_SCANCODE_Q] - key_state[SDL_SCANCODE_E]) * camera_speed,
			(key_state[SDL_SCANCODE_S] - key_state[SDL_SCANCODE_W]) * camera_speed}));
	*/
}
