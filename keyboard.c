#include <SDL2/SDL.h>
#include "keyboard.h"
#include "init.h"
#include "shaders.h"
#include "main.h"
#include "macros.h"

extern SDL_Window *window;
int keys[NUM_HANDLED_KEYS] = {FALSE};
extern int loop_iter_ave;

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
	case SDL_SCANCODE_LEFT:
		keys[KEY_LEFT] = keystate;
		break;
	case SDL_SCANCODE_RIGHT:
		keys[KEY_RIGHT] = keystate;
		break;
	case SDL_SCANCODE_UP:
		keys[KEY_UP] = keystate;
		break;
	case SDL_SCANCODE_DOWN:
		keys[KEY_DOWN] = keystate;
		break;
	case SDL_SCANCODE_EQUALS:
		keys[KEY_EQUALS] = keystate;
		break;
	case SDL_SCANCODE_MINUS:
		keys[KEY_MINUS] = keystate;
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
		if (keystate) {
			int error = init_shaders(shader_programs, shader_infos, LENGTH(shader_programs), TRUE);
			printf("error was: %d\n", error);
		}
		break;
	default: break;
	}
}