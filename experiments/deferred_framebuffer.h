#ifndef DEFERRED_FRAMEBUFFER_H
#define DEFERRED_FRAMEBUFFER_H
#include "graphics.h"
#include <stdio.h>

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

struct accumulation_buffer {
	GLuint fbo;
	GLuint textures[2];
	GLuint depth;
};

struct color_buffer {
	GLuint fbo;
	GLuint texture;
	GLuint depth;
};

struct renderable_cubemap {
	GLuint fbo;
	GLuint texture;
	GLuint depth;
	float width;
};

struct deferred_framebuffer new_deferred_framebuffer(int width, int height);
struct accumulation_buffer new_accumulation_buffer(int width, int height);
struct color_buffer color_buffer_new(int width, int height);
void delete_deferred_framebuffer(struct deferred_framebuffer fb);
void delete_accumulation_buffer(struct accumulation_buffer ab);
void color_buffer_delete(struct color_buffer cb);
void color_buffer_bind_for_reading(struct color_buffer cb);
void bind_deferred_for_reading(struct deferred_framebuffer fb, struct accumulation_buffer ab);
void bind_accumulation_for_reading(struct accumulation_buffer ab);

#define CUBEMAP_FOCAL_LENGTH 1.0 // 1.0/tan(M_PI/4.0), focal length for a 90-degree cubemap face camera is 1.0
struct renderable_cubemap renderable_cubemap_new(int width);
int renderable_cubemap_bind(struct renderable_cubemap rc);

#endif