#include <stdio.h>
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <SDL2_image/SDL_image.h>
#include "global_images.h"
#include "init.h"
#include "default_settings.h"

extern SDL_Window *window;
extern SDL_Renderer *renderer;

int init()
{
	int error = 0;
	if ((error = SDL_Init(SDL_INIT_VIDEO)) < 0) {
		printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
		return error;
	}

	window = SDL_CreateWindow("SDL Tutorial", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
	if (window == NULL) {
		printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
		return -1;
	}
	int img_flags = IMG_INIT_PNG;
	if (!(IMG_Init(img_flags) & img_flags)) {
		printf("SD_image could not initialize! SDL_image Error: %s\n", IMG_GetError());
		return -1;
	}

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	if (renderer == NULL) {
		printf("Renderer could not be created! SDL Error: %s\n", SDL_GetError());
		return -1;
	}
	SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);

	return error;
}

void deinit()
{
	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	IMG_Quit();
	SDL_Quit();
}