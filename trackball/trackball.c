#include "trackball.h"
#include "math/utility.h"
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
		.min_radius = radius / 500.0, //Randomly chosen, no special meaning.
		.max_radius = INFINITY,
		.bounds = {INFINITY, INFINITY, INFINITY, INFINITY}
	};
}

void trackball_set_speed(struct trackball *t, float horizontal, float vertical, float zoom)
{
	t->speed = (vec3){horizontal, vertical, zoom};
}

void trackball_set_target(struct trackball *t, vec3 target)
{
	t->target = target;
	//For now, just reset the camera.
	t->camera.t = (vec3){0, 0, t->radius} + t->target;
	t->camera.a = mat3_lookat(t->camera.t, t->target, (vec3){0, 1, 0}); 
}

void trackball_set_radius(struct trackball *t, float r, float min_r, float max_r)
{
	t->radius = r;
	t->min_radius = min_r;
	t->max_radius = max_r;
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

static void trackball_update(struct trackball *t)
{
	float r = t->radius;
	float x = fclamp(t->rotation.x, -t->bounds.left, t->bounds.right);
	float y = fclamp(t->rotation.y, -t->bounds.bottom, t->bounds.top);
	// float x = t->rotation.x;
	// float y = t->rotation.y;
	//A is a position on the horizontal ring around the target.
	vec3 a = {r * sin(x), 0, r * cos(x)};
	vec3 b = vec3_normalize(a) * r * cos(y);
	b.y = r * sin(y);
	t->camera = (amat4){mat3_lookat(b, t->target, (vec3){0, 1, 0}), b};
}

int trackball_step(struct trackball *t, float mouse_x, float mouse_y, bool button, int scroll_x, int scroll_y)
{
	t->radius = fclamp(t->radius + scroll_y * t->speed.z, t->min_radius, t->max_radius);
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
	} else {
//DRAG START
		if (button) {
			t->mouse.button = true;
			t->mouse.x = mouse_x;
			t->mouse.y = mouse_y;
			t->mouse.scroll.x = scroll_x;
			t->mouse.scroll.y = scroll_y;
		} else {
//NO DRAG
			//Do I actually do anything here?
		}
	}

	if (scroll_y || t->mouse.button) {
		trackball_update(t);
		return 1;
	}
	return 0;
}