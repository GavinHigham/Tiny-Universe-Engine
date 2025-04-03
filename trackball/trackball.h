
#ifndef TRACKBALL_H
#define TRACKBALL_H
#include <glla.h>
#include <stdbool.h>

/*
A trackball should take user input (mouse movement, mouse presses) and translate it into drag actions on a ball or turntable.
This should produce some useful forms of output:
	Rotation matrix for objects being rotated
	Orbiting camera
		position
		target
		lookat matrix

It should be able to limit rotation on various axes, and have configurable easing
(perhaps even "soft edges" if the user tries to drag outside the allowed rotation range).

To create a trackball control like what one finds in a 3D modelling program, I'd also need to be aware of the camera's position relative
to the trackball. I may handle that later, but for now it's outside the scope of the implementation.
*/

struct trackball {
	amat4 camera;
	vec3 target, rotation, speed, prev_rotation;
	float radius, min_radius, max_radius;
	struct {
		float top, bottom, left, right;
	} bounds;
	struct {
		bool button;
		float x, y;
		struct {
			int x, y;
		} scroll;
	} mouse;
};

struct trackball trackball_new(vec3 target, float radius);

void trackball_set_target(struct trackball *t, vec3 target);
void trackball_set_radius(struct trackball *t, float r, float min_r, float max_r);
void trackball_set_speed(struct trackball *t, float horizontal, float vertical, float zoom);
//Radians that the trackball can rotate to the top, bottom, left, or right.
//Values greater than 2pi assume no limits. Recommended to keep top and bottom < pi/2.
void trackball_set_bounds(struct trackball *t, float top, float bottom, float left, float right);
//Returns 1 if the trackball was turned.
int trackball_step(struct trackball *t, float mouse_x, float mouse_y, bool button, int scroll_x, int scroll_y);

#endif