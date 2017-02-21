#include <glla.h>
#include <math.h>
#include "ship_control.h"

void apply_impulse(struct ship_physics *ship, vec3 impulse)
{

}

struct ship_physics ship_control(float dt, struct controller_input input, struct ship_physics ship)
{
	//Set the ship's acceleration using the controller axes.
	ship.acceleration = mat3_multvec(ship.position.a, (vec3){
		ship.speed *  input.leftx,
		ship.speed * -input.lefty,
		ship.speed * (input.rtrigger - input.ltrigger)});

	//Angular velocity is currently determined by how much each axis is deflected.
	float a1 = -input.rightx / 100;
	float a2 = input.righty / 100;
	ship.velocity.a = mat3_rot(mat3_rotmat(0, 0, 1, sin(a1), cos(a1)), 1, 0, 0, sin(a2), cos(a2));

	//Add our acceleration to our velocity to change our speed.
	ship.velocity.t = ship.velocity.t + ship.acceleration * dt;

	//Rotate the ship by applying the angular velocity to the angular orientation.
	ship.position.a = mat3_mult(ship.position.a, ship.velocity.a);

	//Move the ship by applying the velocity to the position.
	ship.position.t = ship.position.t + ship.velocity.t;

	//The ship camera should be positioned 2 up and 8 back relative to the ship's frame.
	ship.locked_camera.t      = amat4_multpoint(ship.position, ship.movable_camera);
	ship.locked_camera_target = amat4_multpoint(ship.position, (vec3){0, 0, -4});

	float camera_ease = 0.9; //Using dt on this gives a jittery camera.
	float target_ease = 0.95;
	ship.eased_camera.t      = vec3_lerp(ship.eased_camera.t,      ship.locked_camera.t,      camera_ease);
	ship.eased_camera_target = vec3_lerp(ship.eased_camera_target, ship.locked_camera_target, target_ease);

	//The eye should look from itself to a point in front of the ship, and its "up" should be "up" from the ship's orientation.
	ship.eased_camera.a = mat3_lookat(
		ship.eased_camera.t,
		ship.eased_camera_target,
		mat3_multvec(ship.position.a, (vec3){0, 1, 0}));

	ship.locked_camera.a = mat3_lookat(
		ship.locked_camera.t,
		ship.locked_camera_target,
		mat3_multvec(ship.position.a, (vec3){0, 1, 0}));

	return ship;
}
