#include <stdbool.h>
#include <SDL2/SDL.h>
#include "input_event.h"
#include "init.h"
#include "shader_utils.h"
#include "renderer.h"

extern SDL_Window *window;
const Uint8 *key_state = NULL;
extern int loop_iter_ave;
extern void reload_effects_void_wrapper();
Sint16 axes[NUM_HANDLED_AXES] = {0};

//I may want to move this somewhere more accessible later, for quitting from a menu and such.
void push_quit()
{
	SDL_Event event;
    event.type = SDL_QUIT;

    SDL_PushEvent(&event); //If it fails, the quit keypress was just eaten ¯\_(ツ)_/¯
}

void init_keyboard()
{
	key_state = SDL_GetKeyboardState(NULL);
}

void keyevent(SDL_Keysym keysym, SDL_EventType type)
{
	static int fullscreen = 0;
	bool keydown;
	if (type == SDL_KEYDOWN) {
		keydown = true;
	} else {
		keydown = false;
	}
	switch (keysym.scancode) {
	case SDL_SCANCODE_ESCAPE:
		push_quit();
		break;
	case SDL_SCANCODE_F:
		if (!keydown) {
			fullscreen ^= SDL_WINDOW_FULLSCREEN_DESKTOP;
			SDL_SetWindowFullscreen(window, fullscreen);
		}
		break;
	case SDL_SCANCODE_R:
		printf("Average # of tight loop iterations after sleep: %d\n", loop_iter_ave);
		break;
	case SDL_SCANCODE_1:
		if (!keydown) {
			renderer_queue_reload();
		}
		break;
	default:
		break;
	}
}

void caxisevent(SDL_Event e)
{
	switch (e.caxis.axis) {
	case SDL_CONTROLLER_AXIS_LEFTX:
		axes[LEFTX] = e.caxis.value; break;
	case SDL_CONTROLLER_AXIS_LEFTY:
		axes[LEFTY] = e.caxis.value; break;
	case SDL_CONTROLLER_AXIS_RIGHTX:
		axes[RIGHTX] = e.caxis.value; break;
	case SDL_CONTROLLER_AXIS_RIGHTY:
		axes[RIGHTY] = e.caxis.value; break;
	case SDL_CONTROLLER_AXIS_TRIGGERLEFT:
		axes[TRIGGERLEFT] = e.caxis.value; break;
	case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
		axes[TRIGGERRIGHT] = e.caxis.value; break;
	}
}

void jaxisevent(SDL_Event e)
{
	switch (e.jaxis.axis) {
	case 0:
		axes[LEFTX] = e.jaxis.value; break;
	case 1:
		axes[LEFTY] = e.jaxis.value; break;
	case 2:
		axes[RIGHTX] = e.jaxis.value; break;
	case 3:
		axes[RIGHTY] = e.jaxis.value; break;
	case 4:
		axes[TRIGGERLEFT] = e.jaxis.value; break;
	case 5:
		axes[TRIGGERRIGHT] = e.jaxis.value; break;
	}
}

void jbuttonevent(SDL_Event e)
{
	if (e.jbutton.type == SDL_JOYBUTTONDOWN) {
		switch (e.jbutton.button) {
		case 8: axes[TRIGGERLEFT] = 32768; break;
		case 9: axes[TRIGGERRIGHT] = 32768; break;
		}
	} else if (e.jbutton.type == SDL_JOYBUTTONUP) {
		switch (e.jbutton.button) {
		case 8: axes[TRIGGERLEFT] = 0; break;
		case 9: axes[TRIGGERRIGHT] = 0; break;
		}
	}
}