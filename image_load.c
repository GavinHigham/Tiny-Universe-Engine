#include <SDL2/SDL.h>
#include <SDL2_image/SDL_image.h>
#include "image_load.h"
#include "global_images.h"

//This should be extern, defined elsewhere. I'm just putting it here because this is dead/dormant code, and I want to compile.
extern SDL_Renderer *renderer;

SDL_Texture * load_texture(char *image_path) {
	SDL_Surface *loaded_surface = NULL;
	SDL_Texture *loaded_texture = NULL;

	loaded_surface = IMG_Load(image_path);
	if (loaded_surface == NULL) {
		printf("Unable to load image %s! SDL_image Error: %s\n", image_path, IMG_GetError());
		return NULL;
	}
	
	loaded_texture = SDL_CreateTextureFromSurface(renderer, loaded_surface);
	if (loaded_texture == NULL) {
		printf("Unable to create texture from %s! SDL Error: %s\n", image_path, SDL_GetError());
	}

	SDL_FreeSurface(loaded_surface);
	return loaded_texture;
}

int load_project_textures()
{
	char *image_path = "planet_sm2.png";
	texture = load_texture(image_path);
	if (texture == NULL) {
		return -1;
	}

	return 0;
}