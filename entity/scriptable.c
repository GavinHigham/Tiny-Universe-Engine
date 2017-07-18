#include "scriptable.h"
#include "physical.h"
#include <stdio.h>

scriptable_callback(noop_script)
{
	//Do nothing.
}

scriptable_callback(camera_script)
{
	Physical *camera = entity->Physical;
	//This won't work in situations where the ship entity is shuffled around.
	Physical *ship = ((Entity *)entity->Scriptable->context)->Physical;

	//float camera_ease = 0.5; //Using dt on this gives a jittery camera.
	//float target_ease = 0.5;
	//ship.eased_camera.t       = amat4_multvec(ship.velocity, ship.eased_camera.t);
	// ship.eased_camera.t      = vec3_lerp(ship.eased_camera.t,      ship.locked_camera.t,      camera_ease);
	// ship.eased_camera_target = vec3_lerp(ship.eased_camera_target, ship.locked_camera_target, target_ease);

	//The eye should look from itself to a point in front of the ship, and its "up" should be "up" from the ship's orientation.
	// ship.eased_camera.a = mat3_lookat(
	// 	mat3_multvec(ship.position.a, ship.eased_camera.t),
	// 	mat3_multvec(ship.position.a, ship.eased_camera_target),
	// 	mat3_multvec(ship.position.a, (vec3){0, 1, 0}));
	camera->origin = ship->origin;
	camera->position.a = mat3_lookat(
		mat3_multvec(ship->position.a, camera->position.t) + mat3_multvec(ship->position.a, ship->position.t), //Look source, relative to ship.
		mat3_multvec(ship->position.a, ship->position.t), //Look target (ship), relative to ship.
		mat3_multvec(ship->position.a, (vec3){0, 1, 0})); //Up vector, relative to ship.
}