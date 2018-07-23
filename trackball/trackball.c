#include "trackball.h"
#include <math.h>
#include <stdio.h>

struct trackball trackball_new(vec3 target, float radius)
{
	return (struct trackball){
		.camera = {MAT3_IDENT, (vec3){0, 0, radius} + target},
		.target = target,
		.rotation = {0, 0, 0},
		.prev_rotation = {0, 0, 0},
		//TODO(Gavin): Figure out some nice default values.
		.speed = {radius/300.0, radius/300.0, radius/300.0},
		.radius = radius,
		.bounds = {INFINITY, INFINITY, INFINITY, INFINITY}
	};
}

void trackball_set_speed(struct trackball *t, float horizontal, float vertical)
{
	t->speed = (vec3){horizontal, vertical, 0};
}

void trackball_set_target(struct trackball *t, vec3 target)
{
	t->target = target;
	//For now, just reset the camera.
	t->camera = (amat4){MAT3_IDENT, (vec3){0, 0, t->radius} + t->target};
}

void trackball_set_radius(struct trackball *t, float r)
{
	t->radius = r;
	t->camera.t = vec3_normalize(t->camera.t) * t->radius;
}

void trackball_set_bounds(struct trackball *t, float top, float bottom, float left, float right)
{
	t->bounds.top    = top;
	t->bounds.bottom = bottom;
	t->bounds.left   = left;
	t->bounds.right  = right;
	//For now, just reset the camera.
	t->camera = (amat4){MAT3_IDENT, (vec3){0, 0, t->radius} + t->target};
}

void trackball_step(struct trackball *t, int mouse_x, int mouse_y, bool button)
{
	if (t->mouse.button) {
		if (button) {
//DRAG CONTINUE
			//printf(".");
			t->rotation = t->prev_rotation + (vec3){t->mouse.x - mouse_x, mouse_y - t->mouse.y, 0} * t->speed;
		} else {
//DRAG END
			t->mouse.button = false;
			t->prev_rotation = t->rotation;
		}

		float r = t->radius;
		float x = fmax(fmin(t->rotation.x, t->bounds.right), -t->bounds.left);
		float y = fmax(fmin(t->rotation.y, t->bounds.top), -t->bounds.bottom);
		// float x = t->rotation.x;
		// float y = t->rotation.y;
		//A is a position on the horizontal ring around the target.
		vec3 a = {r * sin(x), 0, r * cos(x)};
		vec3 b = vec3_normalize(a) * r * cos(y);
		b.y = r * sin(y);
		t->camera = (amat4){mat3_lookat(b, t->target, (vec3){0, 1, 0}), b};
	} else {
//DRAG START
		if (button) {
			t->mouse.button = true;
			t->mouse.x = mouse_x;
			t->mouse.y = mouse_y;
		} else {
//NO DRAG
			//Do I actually do anything here?
		}
	}
}