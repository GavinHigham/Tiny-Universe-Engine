#include <math.h>
#include "ship_control.h"
#include "input_event.h"

void apply_impulse(struct ship_physics *ship, vec3 impulse)
{

}

struct controller_input controller_input_apply_threshold(struct controller_input input, float threshold)
{
	input.leftx = fabs(input.leftx) > threshold ? input.leftx : 0;
	input.lefty = fabs(input.lefty) > threshold ? input.lefty : 0;
	input.rightx = fabs(input.rightx) > threshold ? input.rightx : 0;
	input.righty = fabs(input.righty) > threshold ? input.righty : 0;
	return input;
}

/*
The "locked camera" is expressed in coordinates relative to the ship's position, in the ship's affine frame.
Thus, if the ship points down its own -Z axis, a camera with coordinates {0, 4, 8} is 4 meters above the ship, and 8 meters behind it.

The "eased camera" is similar to the locked camera. The primary difference is that its position only changes by easing to the position of the locked camera.
*/

struct ship_physics ship_control(float dt, struct controller_input input, bool buttons[16], struct ship_physics ship)
{
	input = controller_input_apply_threshold(input, 0.005);
	if (buttons[INPUT_BUTTON_A])
		ship.speed = 1000;
	else
		ship.speed = 10;

	//Set the ship's acceleration using the controller axes.
	ship.acceleration = mat3_multvec(ship.position.a, ship.speed * (vec3){
		 input.leftx,
		-input.lefty,
		buttons[INPUT_BUTTON_L2] - buttons[INPUT_BUTTON_R2]});//ship.speed * (input.rtrigger - input.ltrigger)});

	//Angular velocity is currently determined by how much each axis is deflected.
	float a1 = -input.rightx / 100;
	float a2 = input.righty / 100;
	ship.velocity.a = mat3_rot(mat3_rotmat(0, 0, 1, sin(a1), cos(a1)), 1, 0, 0, sin(a2), cos(a2));

	//Add our acceleration to our velocity to change our speed.
	ship.velocity.t = ship.velocity.t + ship.acceleration * dt;

	if (buttons[INPUT_BUTTON_Y])
		ship.velocity.a = (mat3)MAT3_IDENT;

	if (buttons[INPUT_BUTTON_X])
		ship.velocity.t = (vec3){0, 0, 0};

	//Rotate the ship by applying the angular velocity to the angular orientation.
	ship.position.a = mat3_mult(ship.position.a, ship.velocity.a);

	//Move the ship by applying the velocity to the position.
	if (buttons[INPUT_BUTTON_B])
		ship.position.t = ship.position.t + (10000 * ship.acceleration);
	else
		ship.position.t = ship.position.t + ship.velocity.t;

	float camera_ease = 0.5; //Using dt on this gives a jittery camera.
	float target_ease = 0.5;
	//ship.eased_camera.t       = amat4_multvec(ship.velocity, ship.eased_camera.t);
	ship.eased_camera.t      = vec3_lerp(ship.eased_camera.t,      ship.locked_camera.t,      camera_ease);
	ship.eased_camera_target = vec3_lerp(ship.eased_camera_target, ship.locked_camera_target, target_ease);

	//The eye should look from itself to a point in front of the ship, and its "up" should be "up" from the ship's orientation.
	ship.eased_camera.a = mat3_lookat(
		mat3_multvec(ship.position.a, ship.eased_camera.t),
		mat3_multvec(ship.position.a, ship.eased_camera_target),
		mat3_multvec(ship.position.a, (vec3){0, 1, 0}));

	ship.locked_camera.a = mat3_lookat(
		mat3_multvec(ship.position.a, ship.locked_camera.t),
		mat3_multvec(ship.position.a, ship.locked_camera_target),
		mat3_multvec(ship.position.a, (vec3){0, 1, 0}));

	return ship;
}
