#include "entity/entity_components.h"
#include "space/star_box.h"
#include "space/solar_system.h"

//TODO(Gavin): These should all be encapsulated by an entity that represents the solar system
extern solar_system ssystem;
extern qvec3 solar_system_origin;
extern bool gen_solar_systems;
extern int64_t solar_system_star;

//Uuuuuughh
extern bpos_origin eye_sector;

//Collect these in a camera module?
controllable_callback(camera_control)
{

	// puts("1");
	Physical *camera = entity->physical;
	// puts("2");
	//printf("%p\n", entity->Controllable);
	//printf("%p\n", entity->Controllable->context);
	Physical *ship = ((Entity *)entity->controllable->context)->physical;
	//Translate the camera using WASD.
	float camera_speed = 0.5;
	// puts("3");
	camera->position.t = camera->position.t + //Honestly I just tried things at random until it worked, but here's my guess:
		mat3_multvec(mat3_transp(ship->position.a), // 2) Convert those coordinates from world-space to ship-space.
			mat3_multvec(camera->position.a, (vec3){ // 1) Move relative to the frame pointed at the ship.
			(key_state[SDL_SCANCODE_D] - key_state[SDL_SCANCODE_A]) * camera_speed,
			(key_state[SDL_SCANCODE_Q] - key_state[SDL_SCANCODE_E]) * camera_speed,
			(key_state[SDL_SCANCODE_S] - key_state[SDL_SCANCODE_W]) * camera_speed}));
	// puts("4");

	if (key_state[SDL_SCANCODE_8]) {
		double dist = 0;
		int nearest_star_idx = star_box_find_nearest_star_idx(camera->origin, &dist);
		printf("Camera at "); qvec3_print(camera->origin); printf(", nearest star is %i, distance %f.\n", nearest_star_idx, dist);
	}

	//Hacking this together, should put in the right spot later.
	if (gen_solar_systems) {
		double dist = INFINITY;
		int64_t nearest_star_idx = star_box_find_nearest_star_idx(camera->origin, &dist);
		if (solar_system_star != nearest_star_idx) {
			solar_system_free(ssystem);
			ssystem = solar_system_new(eye_sector);
			solar_system_star = nearest_star_idx;
			solar_system_origin = star_box_get_star_origin(nearest_star_idx);
		}
	}
}

scriptable_callback(camera_script)
{
	Physical *camera = entity->physical;
	//This won't work in situations where the ship entity is shuffled around.
	Physical *ship = ((Entity *)entity->scriptable->context)->physical;

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