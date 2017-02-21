#include <stdbool.h>
#include <SDL2/SDL.h>
#include "keyboard.h"
#include "init.h"
#include "shader_utils.h"
#include "renderer.h"
#include "func_list.h"

extern SDL_Window *window;
const Uint8 *key_state = NULL;
extern int loop_iter_ave;
extern void reload_effects_void_wrapper();

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
			func_list_add(&update_func_list, 1, reload_effects_void_wrapper);
		}
		break;
	default:
		break;
	}
}