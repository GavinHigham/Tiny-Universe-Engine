#ifndef METER_H
#define METER_H
#include <stdbool.h>
#include <inttypes.h>

/*
A simple slider-like meter to help me tweak values. Yeah, imgui and others solve this already, but I'll implement myself for fun :)

Have an array of meters, sorted in z-order. Each meter has a width, height, min value, max value, and current value.
Decoration for now will just be color. Later can include border width, padding, and maybe glow. Should also add labels, possibly just stored directly in the source file as a pixel font.
This font can be used to draw into a texture, which can be used to label a meter.

**UPDATE: Added padding, labels**

Interaction works as follows:

When a click is initiated, the click point is stored, and each meter is checked, starting from the most-foreground one. The first meter to contain the click point is the active meter.
The active meter is brought to the front, and all other meters are shifted down.

**UPDATE: Didn't end up doing the "bring to front" logic**

When a click is finished, it is compared against the click initiation point, and the difference in x (horizontal meter) or y (vertical meter) is used to alter the current value of the active meter.
Making a "point-in-box" meter shouldn't be too difficult either, actually.
I can generate a color-picker fairly easily by interpolating between different colors in HSV, and sampling the click termination point.

I also need some way to retrieve the value in each meter. This will be accomplished in a few ways:
	Make it possible to simply query meter value by meter name.
	Make it possible to give a meter a float pointer, to be changed when the meter is clicked, dragged, or released.
	Make it possible to give a meter a callback and context to call when the meter is clicked, dragged, or released.

Positioning should be automatic, with user-override.

Example usage 1:

	float g_gamma;
	...
	meter_ctx *M = NULL;
	meter_init(M, 800, 600);
	...
	meter_add(M, "Gamma", 100, 20, 1.5, 2.5, 2.2);
	meter_target(M, "Gamma", &g_gamma);
	...
	meter_mouse(M, x, y, mouse_is_down);
	...
	meter_deinit(M);

Example usage 2:
	void update_float_uniform(char *name, enum meter_state, float value, void *context)
	{
		if (meter_state == METER_CLICK_ENDED)
			glUniform1f(*((GLuint *)context), value);
	}
	...
	meter_init(M, 800, 600);
	...
	meter_add(M, "Time", 300, 20, 0.0, 1000.0, 0.0);
	meter_callback(M, "Time", update_float_uniform, &TIME_UNIFORM);
	...
	meter_mouse(M, x, y, mouse_is_down);
	...
	meter_deinit(M);


Optional functions, maybe add later:
	Rename meter (old name, new name)
	Click-cancellation
	Delete all meters
	Draw only some meters
	Move a meter, relative to current position
*/

enum meter_state {
	METER_CLICK_ENDED = 0,
	METER_CLICK_STARTED = 1,
	METER_DRAGGED = 2,
	METER_CLICK_STARTED_OUTSIDE = 3
};

enum meter_flags {
	METER_VALUE_BASED_BORDER_COLOR = 1,
	METER_VALUE_BASED_FILL_COLOR   = 2,
	METER_VALUE_BASED_TEXT_COLOR   = 4,
};

typedef void (*meter_callback_fn)(char *name, enum meter_state state, float value, void *context);

typedef struct meter {
	char *name, *fmt;
	float x, y, min, max, value, snap_increment, *target;
	bool always_snap;
	struct widget_meter_style {
		float width, height, padding;
		uint32_t flags;
	} style;
	struct widget_meter_color {
		unsigned char fill[4], border[4], font[4];
	} color;
	meter_callback_fn callback;
	void *callback_context;
} widget_meter;

struct meter_globals;
typedef int (*meter_renderer_init_fn)(struct meter_globals *meter);
typedef int (*meter_renderer_deinit_fn)(struct meter_globals *meter);
typedef int (*meter_renderer_render_fn)(struct meter_globals *meter);
struct meter_renderer {
	meter_renderer_init_fn init;
	meter_renderer_deinit_fn deinit;
	meter_renderer_render_fn render;
	void *renderer_ctx;
};
typedef struct meter_globals {
	float screen_width, screen_height;
	unsigned int num_meters;
	unsigned int max_meters;
	int last_accessed_index; //Signed so initial value can be -1
	struct meter *meters;
	enum meter_state state;
	char *clicked_meter_name;
	float clicked_x, clicked_y;
	int total_label_chars;
	struct meter_renderer renderer;
} meter_ctx;
//A nearly-raw accessor to a meter's value, should only be used by renderers.
float meter_value(struct meter *m);
//Get the index of the currently-clicked meter (or -1), should only be used by renderers.
int meter_get_index(meter_ctx *M, char *name);
//Returns how full the meter is, from 0.0 to 1.0.
float meter_fraction(struct meter *m);

//Add a new meter. Returns 0 on success.
int meter_add(meter_ctx *M, char *name, float width, float height, float min, float max, float value);
//
// int meter_add_lua
//Change an existing meter. Returns 0 on success.
int meter_change(meter_ctx *M, char *name, float width, float height, float min, float max, float value);
//Move an existing meter to position x, y. Not relative to current position.
int meter_position(meter_ctx *M, char *name, float x, float y);
//Set a callback to be called when an existing meter is clicked, dragged, or released.
//Callback arguments:
// name: name of the meter.
// drag_state: Enum telling if callback was called on click start, drag, or click end.
// value: current value of the meter.
// context: the same context that was provided to meter_callback(...) on callback registration.
int meter_callback(meter_ctx *M, char *name, meter_callback_fn callback, void *context);
//Set a target to receive new values when an existing meter is changed. Sends the current value when called.
int meter_target(meter_ctx *M, char *name, float *target);
//Add a new meter with the same characteristics as an existing meter.
int meter_duplicate(meter_ctx *M, char *name, char *duplicate_name);
//Change the printed label for a meter.
// fmt: A printf-style format string. Can contain format specifiers for two arguments:
//  1. A char * to the meter name.
//  2. The float value of the meter (or meter target).
int meter_label(meter_ctx *M, char *name, char *fmt);
//Change the style of an existing meter.
int meter_style(meter_ctx *M, char *name, unsigned char fill_color[4], unsigned char border_color[4], unsigned char font_color[4], float padding, uint32_t flags);
//Set the snap increment of a meter for when meter_mouse_relative with ctrl_down == true is used.
int meter_snap_increment(meter_ctx *M, char *name, float snap_increment);
//Sets a meter to always snap, as if ctrl_down == true (even if it isn't).
int meter_always_snap(meter_ctx *M, char *name, bool always_snap);
//Return the internal value of an existing meter, or 0 on error.
float meter_get(meter_ctx *M, char *name);
//Fill value with the internal value of an existing meter.
int meter_get_value(meter_ctx *M, char *name, float *value);
//Set the internal value of an existing meter, without triggering its callback.
float meter_raw_set(meter_ctx *M, char *name, float value);
//Delete an existing meter.
int meter_delete(meter_ctx *M, char *name);
//Inform the meter module of current mouse state. Should be done in update function.
//Returns 1 if any meter was clicked, returns 2 if any meter is being dragged.
int meter_mouse(meter_ctx *M, float x, float y, bool mouse_down);
//Same as meter_mouse, but takes two extra args for precision/relative tweaking (shift_down) and snapping (ctrl_down).
int meter_mouse_relative(meter_ctx *M, float x, float y, bool mouse_down, bool shift_down, bool ctrl_down);
//Draw all meters.
int meter_draw_all(meter_ctx *M);
//Init the meter system.
int meter_init(meter_ctx *M, float screen_width, float screen_height, unsigned int max_num_meters, struct meter_renderer renderer);
//Deinit the meter system.
int meter_deinit(meter_ctx *M);
//Inform the meter module of a screen resize.
int meter_resize_screen(meter_ctx *M, float screen_width, float screen_height);

#endif