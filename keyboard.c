#include <SDL2/SDL.h>
#include "keyboard.h"
#include "init.h"
#include "shader_utils.h"
#include "main.h"
#include "macros.h"
#include "render.h"
#include "func_list.h"

extern SDL_Window *window;
const Uint8 *key_state = NULL;
extern int loop_iter_ave;
extern void reload_shaders_void_wrapper();

void init_keyboard()
{
	key_state = SDL_GetKeyboardState(NULL);
}

void keyevent(SDL_Keysym keysym, SDL_EventType type)
{
	static int fullscreen = 0;
	int keystate;
	if (type == SDL_KEYDOWN) {
		keystate = TRUE; //Key is down.
	} else {
		keystate = FALSE; //Key is up.
	}
	switch (keysym.scancode) {
	case SDL_SCANCODE_ESCAPE:
		quit_application();
		break;
	case SDL_SCANCODE_F:
		if (!keystate) {
			fullscreen ^= SDL_WINDOW_FULLSCREEN_DESKTOP;
			SDL_SetWindowFullscreen(window, fullscreen);
		}
		break;
	case SDL_SCANCODE_R:
		printf("Average # of tight loop iterations after sleep: %d\n", loop_iter_ave);
		break;
	case SDL_SCANCODE_1:
		if (!keystate) {
			func_list_add(&update_func_list, 1, reload_shaders_void_wrapper);
		}
		break;
	default:
		break;
	}
}