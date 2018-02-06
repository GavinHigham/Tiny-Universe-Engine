#ifndef IMAGE_LOAD_H
#define IMAGE_LOAD_H

#include <SDL2/SDL.h>

int load_project_textures();
SDL_Texture * load_texture(char *image_path);

#endif