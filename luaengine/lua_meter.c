//TODO: Wrap these all in a nice Lua API

/*

//Returns how full the meter is, from 0.0 to 1.0.
float meter_fraction(struct meter *m);

//Add a new meter. Returns 0 on success.
int meter_add(meter_ctx *M, char *name, float width, float height, float min, float value, float max);
//
// int meter_add_lua
//Change an existing meter. Returns 0 on success.
int meter_change(meter_ctx *M, char *name, float width, float height, float min, float value, float max);
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



From Lua:

Meter

*/