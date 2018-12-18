#include "meter.h"
#include "macros.h"
#include "math/utility.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static struct meter_globals meter = {0};

char *default_label = "%s: %.2f";

static int meter_check_enclosing(struct meter *m, float x, float y)
{
	return x >= m->x && y >= m->y && x <= (m->x + m->style.width) && y <= (m->y + m->style.height);
}

static int meter_find_enclosing(float x, float y)
{
	for (int i = 0; i < meter.num_meters; i++)
		if (meter_check_enclosing(&meter.meters[i], x, y))
			return i;
	return -1;
}

static int meter_get_index(char *name)
{
	for (int i = 0; i < meter.num_meters; i++)
		if (name == meter.meters[i].name || !strcmp(name, meter.meters[i].name))
			return i;
	return -1;
}

#define METER_GET_INDEX_OR_RET_ERROR(name, index_name) int index_name = meter_get_index(name); if (mi < 0) return mi;

static void meter_update(struct meter *m, float value)
{
	m->value = value;
	if (m->target)
		*(m->target) = m->value;
	if (m->callback)
		m->callback(m->name, meter.state, m->value, m->callback_context);
}

int meter_add(char *name, float width, float height, float min, float value, float max)
{
	if (meter.num_meters >= meter.max_meters)
		return -1;

	meter.total_label_chars += strlen(name);
	meter.meters[meter.num_meters++] = (struct meter){
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
		.target = NULL,
		.color = {
			.fill = {187, 187, 187, 255},
			.border = {95, 95, 95, 255},
			.font = {255, 255, 255}
		},
		.callback = NULL,
		.callback_context = NULL,
	};
	return 0;
}

int meter_change(char *name, float width, float height, float min, float max, float value)
{
	METER_GET_INDEX_OR_RET_ERROR(name, mi)

	struct meter *m = &meter.meters[mi];
	m->style.width = width;
	m->style.height = height;
	m->min = min;
	m->max = max;
	m->value = value;
	meter_update(m, m->value);

	return 0;
}

int meter_position(char *name, float x, float y)
{
	METER_GET_INDEX_OR_RET_ERROR(name, mi)

	meter.meters[mi].x = x;
	meter.meters[mi].y = y;	
	return 0;
}

int meter_callback(char *name, meter_callback_fn callback, void *context)
{
	METER_GET_INDEX_OR_RET_ERROR(name, mi)

	meter.meters[mi].callback = callback;
	meter.meters[mi].callback_context = context;	
	return 0;
}

int meter_target(char *name, float *target)
{
	METER_GET_INDEX_OR_RET_ERROR(name, mi)

	struct meter *m = &meter.meters[mi];
	m->target = target;
	//In this case, we don't want to use meter_value(m), since the target may not be initialized.
	meter_update(m, m->value);
	return 0;
}

int meter_duplicate(char *name, char *duplicate_name)
{
	METER_GET_INDEX_OR_RET_ERROR(name, mi)

	meter.meters[meter.num_meters++] = meter.meters[mi];
	meter.meters[meter.num_meters - 1].name = strdup(name);
	return 0;
}

int meter_label(char *name, char *fmt)
{
	METER_GET_INDEX_OR_RET_ERROR(name, mi)

	meter.meters[mi].fmt = fmt;
	return 0;
}

int meter_style(char *name, unsigned char fill_color[4], unsigned char border_color[4], unsigned char font_color[4], float padding)
{
	METER_GET_INDEX_OR_RET_ERROR(name, mi)

	memcpy(meter.meters[mi].color.fill,   fill_color,   4 * sizeof(unsigned char));
	memcpy(meter.meters[mi].color.border, border_color, 4 * sizeof(unsigned char));
	memcpy(meter.meters[mi].color.font,   font_color,   4 * sizeof(unsigned char));
	return 0;
}

float meter_value(struct meter *m)
{
	return m->target ? *(m->target) : m->value;
}

float meter_get(char *name)
{
	int mi = meter_get_index(name);
	if (mi < 0)
		return 0;

	return meter_value(&meter.meters[mi]);
}

int meter_get_value(char *name, float *value)
{
	METER_GET_INDEX_OR_RET_ERROR(name, mi)
	struct meter *m = &meter.meters[mi];

	*value = meter_value(m);
	return 0;
}

int meter_delete(char *name)
{
	METER_GET_INDEX_OR_RET_ERROR(name, mi)

	meter.total_label_chars -= strlen(meter.meters[mi].name);
	free(meter.meters[mi].name);
	//Shift meters down, taking the place of the deleted meter.
	memmove(&meter.meters[mi], &meter.meters[mi+1], sizeof(struct meter) * (meter.num_meters - mi - 1));
	meter.num_meters--;
	return 0;
}

int meter_mouse(float x, float y, bool mouse_down)
{
	if (meter.state == METER_CLICK_STARTED || meter.state == METER_DRAGGED) {
		//PERF(Gavin): If I want to make this faster in the future, I can directly compare the char pointers,
		//since I store the actual pointer of the clicked meter, instead of a duplicate.
		int mi = meter_get_index(meter.clicked_meter_name);
		if (mi < 0) {
			//Got into a weird state, correct it.
			meter.state = METER_CLICK_ENDED;
			meter.clicked_meter_name = NULL;
			return 0;
		}

		if (mouse_down)
			meter.state = METER_DRAGGED;
		else
			meter.state = METER_CLICK_ENDED;

		struct meter *m = &meter.meters[mi];
		float pad_x_left = m->x + m->style.padding;
		float pad_x_right = m->x + m->style.width - 2 * m->style.padding;
		float value = (x - pad_x_left) / (pad_x_right - pad_x_left) * (m->max - m->min) + m->min;
		value = fclamp(value, m->min, m->max);
		meter_update(m, value);
		//printf("Meter dragged, set to %f\n", meter_value(m));
		if (METER_DRAGGED)
			return 2;
	} else if (meter.state == METER_CLICK_ENDED) {
		if (mouse_down) {
			int mi = meter_find_enclosing(x, y);
			if (mi < 0) {
				meter.state = METER_CLICK_STARTED_OUTSIDE;
				return 0;
			}

			meter.state = METER_CLICK_STARTED;
			struct meter *m = &meter.meters[mi];
			meter.clicked_meter_name = m->name;
			meter.clicked_x = x;
			meter.clicked_y = y;

			//Pass a different value if I want absolute instead of relative sliding.
			meter_update(m, meter_value(m));
			return 1;
		}
	} else if (meter.state == METER_CLICK_STARTED_OUTSIDE) {
		if (!mouse_down)
			meter.state = METER_CLICK_ENDED;
	}
	return 0;
}

int meter_draw_all()
{
	return meter.renderer.render(&meter);
}

void meter_glyph_meter_callback(char *name, enum meter_state state, float value, void *context)
{
	float *target = context;
	*target = (int)value;
}

//Init the meter system.
// int meter_init(float screen_width, float screen_height, unsigned int max_num_meters)
int meter_init(float screen_width, float screen_height, unsigned int max_num_meters, struct meter_renderer renderer)
{
	meter = (struct meter_globals){
		.screen_width = screen_width,
		.screen_height = screen_height,
		.num_meters = 0,
		.max_meters = max_num_meters,
		.meters = malloc(max_num_meters * sizeof(struct meter)),
		.renderer = renderer
	};

	if (meter.renderer.init(&meter) < 0) {
		meter_deinit();
		goto error;
	}

	return 0;
error:
	printf("Error initializing meter.\n");
	return -1;
}

int meter_deinit()
{
	for (int i = 0; i < meter.num_meters; i++)
		free(meter.meters[i].name);
	free(meter.meters);

	meter.renderer.deinit(&meter);
	return 0;
}

int meter_resize_screen(float screen_width, float screen_height)
{
	meter.screen_width = screen_width;
	meter.screen_height = screen_height;
	//Possibly reflow meters?
	return 0;
}