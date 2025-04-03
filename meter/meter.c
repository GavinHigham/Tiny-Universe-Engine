#include "meter.h"
#include "macros.h"
#include "math/utility.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

char *default_label = "%s: %.2f";

static int meter_check_enclosing(meter_ctx *M, struct meter *m, float x, float y)
{
	return x >= m->x && y >= m->y && x <= (m->x + m->style.width) && y <= (m->y + m->style.height);
}

static int meter_find_enclosing(meter_ctx *M, float x, float y)
{
	for (int i = 0; i < M->num_meters; i++)
		if (meter_check_enclosing(M, &M->meters[i], x, y))
			return i;
	return -1;
}

int meter_get_index(meter_ctx *M, char *name)
{
	if (!name)
		return -1;
	if (M->last_accessed_index >= 0) {
		char *last_accessed_name = M->meters[M->last_accessed_index].name;
		if (name == last_accessed_name || !strcmp(name, last_accessed_name))
			return M->last_accessed_index;
	}
	for (int i = 0; i < M->num_meters; i++) {
		if (name == M->meters[i].name || !strcmp(name, M->meters[i].name)) {
			M->last_accessed_index = i;
			return i;
		}
	}
	return -1;
}

#define METER_GET_INDEX_OR_RET_ERROR(ctx, name, index_name) int index_name = meter_get_index(ctx, name); if (mi < 0) return mi;

static void meter_update(meter_ctx *M, struct meter *m, float value, bool trigger_callback)
{
	m->value = value;
	if (m->target)
		*(m->target) = m->value;
	if (m->callback && trigger_callback)
		m->callback(m->name, M->state, m->value, m->callback_context);
}

int meter_add(meter_ctx *M, char *name, float width, float height, float min, float value, float max)
{
	if (M->num_meters >= M->max_meters)
		return -1;

	M->total_label_chars += strlen(name);
	M->meters[M->num_meters++] = (struct meter){
		.name = strdup(name),
		.fmt = default_label,
		.x = 0, //CHANGE THIS
		.y = 0, //CHANGE THIS
		.style = {
			.width = width,
			.height = height,
			.padding = 3.0
		},
		.min = min,
		.max = max,
		.value = value,
		.snap_increment = 1.0,
		.target = NULL,
		.color = {
			.fill = {187, 187, 187, 255},
			.border = {95, 95, 95, 255},
			.font = {255, 255, 255}
		},
		.callback = NULL,
		.callback_context = NULL,
	};
	M->last_accessed_index = M->num_meters - 1;
	return 0;
}

int meter_change(meter_ctx *M, char *name, float width, float height, float min, float value, float max)
{
	METER_GET_INDEX_OR_RET_ERROR(M, name, mi)

	struct meter *m = &M->meters[mi];
	m->style.width = width;
	m->style.height = height;
	m->min = min;
	m->max = max;
	m->value = value;
	meter_update(M, m, m->value, true);

	return 0;
}

int meter_position(meter_ctx *M, char *name, float x, float y)
{
	METER_GET_INDEX_OR_RET_ERROR(M, name, mi)

	M->meters[mi].x = x;
	M->meters[mi].y = y;	
	return 0;
}

int meter_callback(meter_ctx *M, char *name, meter_callback_fn callback, void *context)
{
	METER_GET_INDEX_OR_RET_ERROR(M, name, mi)

	M->meters[mi].callback = callback;
	M->meters[mi].callback_context = context;	
	return 0;
}

int meter_target(meter_ctx *M, char *name, float *target)
{
	METER_GET_INDEX_OR_RET_ERROR(M, name, mi)

	struct meter *m = &M->meters[mi];
	m->target = target;
	//In this case, we don't want to use meter_value(m), since the target may not be initialized.
	meter_update(M, m, m->value, true);
	return 0;
}

int meter_duplicate(meter_ctx *M, char *name, char *duplicate_name)
{
	METER_GET_INDEX_OR_RET_ERROR(M, name, mi)

	M->meters[M->num_meters++] = M->meters[mi];
	M->meters[M->num_meters - 1].name = strdup(name);
	return 0;
}

int meter_label(meter_ctx *M, char *name, char *fmt)
{
	METER_GET_INDEX_OR_RET_ERROR(M, name, mi)

	M->meters[mi].fmt = fmt;
	return 0;
}

int meter_style(meter_ctx *M, char *name, unsigned char fill_color[4], unsigned char border_color[4], unsigned char font_color[4], float padding, uint32_t flags)
{
	METER_GET_INDEX_OR_RET_ERROR(M, name, mi)

	memcpy(M->meters[mi].color.fill,   fill_color,   4 * sizeof(unsigned char));
	memcpy(M->meters[mi].color.border, border_color, 4 * sizeof(unsigned char));
	memcpy(M->meters[mi].color.font,   font_color,   4 * sizeof(unsigned char));
	M->meters[mi].style.flags = flags; //If I later have non-style flags, I can split this out into bools internally.
	return 0;
}

int meter_snap_increment(meter_ctx *M, char *name, float snap_increment)
{
	METER_GET_INDEX_OR_RET_ERROR(M, name, mi)

	M->meters[mi].snap_increment = snap_increment;
	return 0;
}

int meter_always_snap(meter_ctx *M, char *name, bool always_snap)
{
	METER_GET_INDEX_OR_RET_ERROR(M, name, mi)

	M->meters[mi].always_snap = always_snap;
	return 0;
}

float meter_value(struct meter *m)
{
	return m->target ? *(m->target) : m->value;
}

float meter_fraction(struct meter *m)
{
	return (m->value - m->min)/(m->max - m->min);
}

float meter_get(meter_ctx *M, char *name)
{
	int mi = meter_get_index(M, name);
	if (mi < 0)
		return 0;

	return meter_value(&M->meters[mi]);
}

int meter_get_value(meter_ctx *M, char *name, float *value)
{
	METER_GET_INDEX_OR_RET_ERROR(M, name, mi)
	struct meter *m = &M->meters[mi];

	*value = meter_value(m);
	return 0;
}

float meter_raw_set(meter_ctx *M, char *name, float value)
{
	METER_GET_INDEX_OR_RET_ERROR(M, name, mi)
	meter_update(M, &M->meters[mi], value, false);

	return 0;
}

int meter_delete(meter_ctx *M, char *name)
{
	METER_GET_INDEX_OR_RET_ERROR(M, name, mi)

	M->total_label_chars -= strlen(M->meters[mi].name);
	free(M->meters[mi].name);
	//Shift meters down, taking the place of the deleted M->
	memmove(&M->meters[mi], &M->meters[mi+1], sizeof(struct meter) * (M->num_meters - mi - 1));
	M->num_meters--;
	return 0;
}

int meter_mouse(meter_ctx *M, float x, float y, bool mouse_down)
{
	return meter_mouse_relative(M, x, y, mouse_down, false, false);
}

int meter_mouse_relative(meter_ctx *M, float x, float y, bool mouse_down, bool shift_down, bool ctrl_down)
{
	if (M->state == METER_CLICK_STARTED || M->state == METER_DRAGGED) {
		int mi = meter_get_index(M, M->clicked_meter_name);
		if (mi < 0) {
			//Got into a weird state, correct it.
			M->state = METER_CLICK_ENDED;
			M->clicked_meter_name = NULL;
			return 0;
		}

		if (mouse_down)
			M->state = METER_DRAGGED;
		else
			M->state = METER_CLICK_ENDED;

		struct meter *m = &M->meters[mi];

		float left_edge = m->x + m->style.padding;
		float right_edge = m->x + m->style.width - 2 * m->style.padding;
		float value_range = m->max - m->min;
		float fill_width = right_edge - left_edge;
		float clicked_value = (x - left_edge) * value_range / fill_width + m->min;
		ctrl_down = ctrl_down || m->always_snap;

		if (shift_down && !ctrl_down) {
			//When shift is held, right-edge of filled region slowly drifts towards mouse cursor,
			//with velocity proportional to cursor distance from right-edge of filled region
			meter_update(M, m, fclamp(0.999 * m->value + 0.001 * clicked_value, m->min, m->max), true);
		} else {
			//With no modifiers, clicking the meter just directly sets the value.
			//Ex. Clicking 25% between the left and right edge gives a value 25% between the configured min and max value.
			float value = fclamp(clicked_value, m->min, m->max);
			//If the ctrl modifier is held, snap to the "snap increment", which defaults to 1.0.
			if (ctrl_down) {
				float increment = m->snap_increment;
				//If both ctrl and shift are held, snap to 1/10th the "snap increment".
				if (shift_down)
					increment /= 10.0;
				value = floor(value / increment) * increment;
			}
			meter_update(M, m, value, true);
		}
		//printf("Meter dragged, set to %f\n", meter_value(m));
		if (METER_DRAGGED)
			return 2;
	} else if (M->state == METER_CLICK_ENDED) {
		if (mouse_down) {
			int mi = meter_find_enclosing(M, x, y);
			if (mi < 0) {
				M->state = METER_CLICK_STARTED_OUTSIDE;
				return 0;
			}

			M->state = METER_CLICK_STARTED;
			struct meter *m = &M->meters[mi];
			M->clicked_meter_name = m->name;
			M->clicked_x = x;
			M->clicked_y = y;

			//Pass a different value if I want absolute instead of relative sliding.
			meter_update(M, m, meter_value(m), true);
			return 1;
		}
	} else if (M->state == METER_CLICK_STARTED_OUTSIDE) {
		if (!mouse_down)
			M->state = METER_CLICK_ENDED;
	}
	return 0;
}

int meter_draw_all(meter_ctx *M)
{
	return M->renderer.render(M);
}

//Init the meter system.
int meter_init(meter_ctx *M, float screen_width, float screen_height, unsigned int max_num_meters, struct meter_renderer renderer)
{
	*M = (struct meter_globals){
		.screen_width = screen_width,
		.screen_height = screen_height,
		.num_meters = 0,
		.max_meters = max_num_meters,
		.last_accessed_index = -1,
		.meters = malloc(max_num_meters * sizeof(struct meter)),
		.renderer = renderer
	};

	if (M->renderer.init(M) < 0) {
		meter_deinit(M);
		goto error;
	}

	return 0;
error:
	printf("Error initializing meter.\n");
	return -1;
}

int meter_deinit(meter_ctx *M)
{
	for (int i = 0; i < M->num_meters; i++)
		free(M->meters[i].name);
	free(M->meters);

	M->renderer.deinit(M);
	return 0;
}

int meter_resize_screen(meter_ctx *M, float screen_width, float screen_height)
{
	M->screen_width = screen_width;
	M->screen_height = screen_height;
	//Possibly reflow meters?
	return 0;
}