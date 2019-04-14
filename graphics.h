#ifndef GRAPHICS_H
#define GRAPHICS_H

#ifdef __EMSCRIPTEN__
	#include <emscripten.h>
	#include <GLES2/gl2.h>
	#define GL_GEOMETRY_SHADER 42 //Just to get this to compile
#else
	// #ifdef _WIN32
	//     #define APIENTRY __stdcall
	// #endif
	// #include "glad/glad.h"
	#include <GL/glew.h>
#endif
#define CHECK_ERRORS() {int error = glGetError(); if (error != GL_NO_ERROR) printf("%s[%d]: %d\n", __FUNCTION__, __LINE__, error);}

#include <SDL2/SDL.h>
#include <SDL2_image/SDL_image.h>

#endif