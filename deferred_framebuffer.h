#ifndef DEFERRED_FRAMEBUFFER_H
#define DEFERRED_FRAMEBUFFER_H
#include <stdio.h>
#include <GL/glew.h>

enum GBUFFER_TEXTURE_TYPE {
	GBUFFER_TEXTURE_TYPE_POSITION,
	GBUFFER_TEXTURE_TYPE_DIFFUSE,
	GBUFFER_TEXTURE_TYPE_NORMAL,
	GBUFFER_TEXTURE_TYPE_TEXCOORD,
	GBUFFER_NUM_TEXTURES
};

struct deferred_framebuffer {
	GLuint fbo;
	GLuint textures[GBUFFER_NUM_TEXTURES];
	GLuint depth;
};

struct deferred_framebuffer new_deferred_framebuffer(int width, int height);
void delete_deferred_framebuffer(struct deferred_framebuffer fb);
void bind_deferred_for_reading(struct deferred_framebuffer fb);

#endif