#include <stdio.h>
#include <stdbool.h>
#include <signal.h>
#include <string.h>
//#include <SDL2/SDL_opengl.h>
//Lua headers
#include <lua-5.4.4/src/lua.h>
#include <lua-5.4.4/src/lauxlib.h>
#include <lua-5.4.4/src/lualib.h>
//Project headers.
#include "graphics.h"
#include "init.h"
#include "math/utility.h"
#include "input_event.h"
#include "macros.h"
//Scenes
#include "space/space_scene.h"
#include "luaengine/lua_scene.h"
#include "experiments/icosphere_scene.h"
#include "experiments/proctri_scene.h"
#include "experiments/spiral_scene.h"
#include "experiments/visualizer_scene.h"
#include "experiments/atmosphere/atmosphere_scene.h"
#include "experiments/universe_scene/universe_scene.h"
#include "experiments/spawngrid/spawngrid_scene.h"
//Tests
#include "test/test_main.h"
//Configuration
#include "luaengine/lua_configuration.h"

static const int MS_PER_SECOND = 1000;
static const int FRAMES_PER_SECOND = 60; //Frames per second.
//static const int MS_PER_UPDATE = MS_PER_SECOND / FRAMES_PER_SECOND;
static int wake_early_ms = 2; //How many ms early should the main loop wake from sleep to avoid oversleeping.
static int loop_iter_ave = 0; //Average number loop iterations we burn after sleep, waiting to run update and render.
static bool testmode = false; //If true, skip creating the window and just run the tests.
static SDL_Window *window = NULL;
static uint32_t windowID = 0;
static const char *luaconf_path = "conf.lua";
static bool ffmpeg_recording = false;
static int *ffmpeg_buffer = NULL;
static FILE *ffmpeg_file;

lua_State *L = NULL;
char *data_path = NULL;

void global_keys(SDL_Keysym keysym, SDL_EventType type)
{
	static int fullscreen = 0;
	switch (keysym.scancode) {
	case SDL_SCANCODE_ESCAPE:
		SDL_PushEvent(&(SDL_Event){.type = SDL_QUIT}); //If it fails, the quit keypress was just eaten ¯\_(ツ)_/¯
		break;
	case SDL_SCANCODE_F:
		if (key_pressed(keysym.scancode)) {
			fullscreen ^= SDL_WINDOW_FULLSCREEN_DESKTOP;
			SDL_SetWindowFullscreen(window, fullscreen);
		}
		break;
	case SDL_SCANCODE_R:
		if ((key_down(SDL_SCANCODE_LSHIFT) || key_down(SDL_SCANCODE_RSHIFT)) && key_pressed(keysym.scancode)) {
			ffmpeg_recording = !ffmpeg_recording;
			if (ffmpeg_recording) {
				printf("Starting to record!\n");
				char *cmd = getglobstr(L, "ffmpeg_cmd", "output_error.txt");
				ffmpeg_file = popen(cmd, "w");
				free(cmd);
				if (!ffmpeg_file)
					printf("Could not open ffmpeg file.\n");
				ffmpeg_buffer = malloc(sizeof(int) * getglob(L, "screen_width", 800) * getglob(L, "screen_height", 600));
				if (!ffmpeg_buffer)
					printf("Could not allocate memory.\n");
			}
			else {
				printf("Stopping recording!\n");
				pclose(ffmpeg_file);
				free(ffmpeg_buffer);
			}
		}
		break;
	case SDL_SCANCODE_1:
		if (key_pressed(keysym.scancode))
			scene_reload();
		break;
	default:
		break;
	}
}

//Returns false if there has been a quit event, otherwise returns true
bool drain_event_queue()
{
	mousewheelreset();
	SDL_Event e;
	while (SDL_PollEvent(&e)) {
		switch (e.type) {
		case SDL_QUIT:                  return false;
		case SDL_KEYDOWN:               global_keys(e.key.keysym, (SDL_EventType)e.type); break;
		case SDL_KEYUP:                 global_keys(e.key.keysym, (SDL_EventType)e.type); break;
		case SDL_CONTROLLERAXISMOTION:  caxisevent(e); break;
		case SDL_JOYAXISMOTION:         jaxisevent(e); break;
		case SDL_JOYBUTTONDOWN:         jbuttonevent(e); break;
		case SDL_JOYBUTTONUP:           jbuttonevent(e); break;
		case SDL_MOUSEWHEEL:			mousewheelevent(e); break;
		case SDL_CONTROLLERDEVICEADDED: //Fall-through
		case SDL_JOYDEVICEADDED:        input_event_device_arrival(e.jdevice.which); break;
		case SDL_WINDOWEVENT:
			if (e.window.windowID == windowID) {
				switch (e.window.event)  {
				case SDL_WINDOWEVENT_SIZE_CHANGED:
					scene_resize(e.window.data1, e.window.data2);
					break;
				}
			} break;
		case SDL_DROPFILE: {
				// SDL_ShowSimpleMessageBox(
				// 	SDL_MESSAGEBOX_INFORMATION, "File dropped on window", e.drop.file, window);
				scene_filedrop(e.drop.file);
				SDL_free(e.drop.file);
			} break;
		}
	}
	return true;
}

//Signal handler that tells the renderer module to reload itself.
static void reload_signal_handler(int signo) {
	printf("Received SIGUSR1! Reloading shaders!\n");
	luaconf_run(L, data_path, luaconf_path);
	scene_reload();
}

int main(int argc, char **argv)
{	
	char *arg1 = NULL;
	if (argc > 1)
		arg1 = argv[1];

	if (arg1 && !strcmp(arg1, "test"))
		testmode = true;

	int result = 0;
	if ((result = SDL_Init(SDL_INIT_EVERYTHING)) != 0) {
		fprintf(stderr, "SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
		return result;
	}

	//Get a base path for any application files (shaders, scripts)
	data_path = SDL_GetBasePath();
	if (!data_path)
		data_path = SDL_strdup("./");

	L = luaL_newstate();
	luaL_openlibs(L);
	lua_pushstring(L, data_path);
	lua_setglobal(L, "data_path");
	if (arg1) {
		lua_pushstring(L, arg1);
		lua_setglobal(L, "manual_scene");
	}
	luaconf_run(L, data_path, luaconf_path);
	luaconf_run(L, data_path, "libraries.lua");

	char *screen_title = getglob(L, "screen_title", "Creative Title");
	char *default_scene = getglob(L, "default_scene", "icosphere_scene");
	float screen_width = getglob(L, "screen_width", 800);
	float screen_height = getglob(L, "screen_height", 600);
	bool fullscreen = getglobbool(L, "fullscreen", false);
	bool highdpi = getglobbool(L, "allow_highdpi", false);
	SDL_SetRelativeMouseMode(getglobbool(L, "grab_mouse", false));

	SDL_GLContext context = NULL;
	if (!testmode) { //Skip window creation, OpenGL init, and and GLEW init in test mode.
		window = SDL_CreateWindow(
		screen_title,
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		screen_width,
		screen_height,
		SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | (highdpi ? SDL_WINDOW_ALLOW_HIGHDPI : 0));

		if (window == NULL) {
			fprintf(stderr, "Window could not be created! SDL_Error: %s\n", SDL_GetError());
			result = -2;
			goto error;
		}
		SDL_EventState(SDL_DROPFILE, SDL_ENABLE);

		if (gl_init(&context, window)) {
			fprintf(stderr, "OpenGL could not be initiated!\n");
			result = -3;
			goto error;
		}
	}

	int drawable_width = screen_width;
	int drawable_height = screen_height;
	SDL_GL_GetDrawableSize(window, &drawable_width, &drawable_height);

	result = engine_init();
	if (result < 0) {
		fprintf(stderr, "Something went wrong in init_engine! Aborting.\n");
		result = -4;
		goto error;
	}

	if (testmode) {
		result = test_main(argc, argv);
		goto error;
	}
	
	//When we receive SIGUSR1, reload the scene.
	if (signal(SIGUSR1, reload_signal_handler) == SIG_ERR) {
		fprintf(stderr, "An error occurred while setting a signal handler.\n");
	}

	struct game_scene *scenes[] = {
		&space_scene,
		&proctri_scene,
		&spiral_scene,
		&icosphere_scene,
		&visualizer_scene,
		&atmosphere_scene,
		&universe_scene,
		&spawngrid_scene,
		&lua_scene,
	};

	//Scenes usually depend on OpenGL being init'd.
	char *chosen_scene = arg1 ? arg1 : default_scene;
	for (int i = 0; i < LENGTH(scenes); i++)
		if (!strcmp(chosen_scene, scenes[i]->name))
			scene_set(scenes[i]);
	scene_resize(drawable_width, drawable_height);

	if (fullscreen)
		SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);

	windowID = SDL_GetWindowID(window);
	uint32_t last_swap_timestamp = SDL_GetTicks();
	int loop_iter = 0;
	while (drain_event_queue()) { //Loop until quit event detected
		loop_iter++; //Count how many times we loop per frame.

		int frame_time_ms = MS_PER_SECOND/FRAMES_PER_SECOND;
		uint32_t since_update_ms = (SDL_GetTicks() - last_swap_timestamp);

		if (since_update_ms >= frame_time_ms - 1) {
				//Since user input is handled above, game state is "locked" when we enter this block.
				scene_update(1.0/FRAMES_PER_SECOND); //At 16 ms intervals, begin an update. HOPEFULLY DOESN'T TAKE MORE THAN 16 MS.
				scene_render(); //This will be a picture of the state as of (hopefully exactly) 16 ms ago.
		 		SDL_GL_SwapWindow(window); //Display a new screen to the user every 16 ms, on the dot.
				last_swap_timestamp = SDL_GetTicks();

		 		if (ffmpeg_recording && ffmpeg_buffer && ffmpeg_file) {
		 			int width = getglob(L, "screen_width", screen_width), height = getglob(L, "screen_height", screen_height);
			 		glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, ffmpeg_buffer);
			 		fwrite(ffmpeg_buffer, sizeof(int)*width*height, 1, ffmpeg_file);
			 		printf(".");
		 		}
				//Get a rolling average of the number of tight loop iterations per frame.
				loop_iter_ave = (loop_iter_ave + loop_iter)/2; //Average the current number of loop iterations with the average.
				loop_iter = 0;

				//Needs to be done before the call to SDL_PollEvent (which implicitly calls SDL_PumpEvents)
				//WARNING: This modifies the input state.
				//Done here because there are sometimes issues detecting "pressed" edges from rapid keypresses if it's placed after the SDL_Delay
				input_event_save_prev_key_state();
				input_event_save_prev_mouse_state();
		} else if ((frame_time_ms - since_update_ms) > wake_early_ms) { //If there's more than wake_early_ms milliseconds left...
			SDL_Delay(frame_time_ms - since_update_ms - wake_early_ms); //Sleep up until wake_early_ms milliseconds left. (Busywait the rest)
		}
	}

	// From http://gameprogrammingpatterns.com/game-loop.html
	// uint32_t previous = SDL_GetTicks();
	// double lag = 0.0;
	// while (!quit)
	// {
	// 	uint32_t current = SDL_GetTicks();
	// 	uint32_t elapsed = current - previous;
	// 	previous = current;
	// 	lag += elapsed;

	// 	drain_event_queue();

	// 	while (lag >= MS_PER_UPDATE)
	// 	{
	// 		update(MS_PER_UPDATE/MS_PER_SECOND);
	// 		lag -= MS_PER_UPDATE;
	// 	}
	// 	render();
	// 	SDL_GL_SwapWindow(window);
	// }

error:
	scene_set(NULL);
	SDL_free(data_path);
	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(window);
	engine_deinit();
	free(screen_title);
	free(default_scene);
	return result;
}
