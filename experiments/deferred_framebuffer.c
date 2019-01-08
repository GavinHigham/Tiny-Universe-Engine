#include "deferred_framebuffer.h"
#include "../macros.h"
#include "graphics.h"

struct deferred_framebuffer new_deferred_framebuffer(int width, int height)
{
	struct deferred_framebuffer tmp;
	glGenFramebuffers(1, &tmp.fbo);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, tmp.fbo);

	glGenTextures(GBUFFER_NUM_TEXTURES, tmp.textures);
	glGenTextures(1, &tmp.depth);

	for (int i = 0 ; i < GBUFFER_NUM_TEXTURES; i++) {
		glBindTexture(GL_TEXTURE_2D, tmp.textures[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, width, height, 0, GL_RGB, GL_FLOAT, NULL);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, tmp.textures[i], 0);
	}

	glBindTexture(GL_TEXTURE_2D, tmp.depth);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, tmp.depth, 0);

	GLenum draw_buffers[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3}; 
	glDrawBuffers(LENGTH(draw_buffers), draw_buffers);

	GLenum error = glCheckFramebufferStatus(GL_FRAMEBUFFER);

	if (error != GL_FRAMEBUFFER_COMPLETE) {
		printf("FB error, status: 0x%x\n", error);
	}

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

	return tmp;
}

struct accumulation_buffer new_accumulation_buffer(int width, int height)
{
	struct accumulation_buffer tmp;
	glGenFramebuffers(1, &tmp.fbo);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, tmp.fbo);

	glGenTextures(LENGTH(tmp.textures), tmp.textures);
	glGenTextures(1, &tmp.depth);
	
	for (int i = 0 ; i < LENGTH(tmp.textures); i++) {
		glBindTexture(GL_TEXTURE_2D, tmp.textures[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, NULL);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, tmp.textures[i], 0);
	}

	glBindTexture(GL_TEXTURE_2D, tmp.depth);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, tmp.depth, 0);

	GLenum draw_buffers[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1}; 
	glDrawBuffers(LENGTH(draw_buffers), draw_buffers);

	GLenum error = glCheckFramebufferStatus(GL_FRAMEBUFFER);

	if (error != GL_FRAMEBUFFER_COMPLETE) {
		printf("FB error, status: 0x%x\n", error);
	}

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

	return tmp;
}

struct color_buffer color_buffer_new(int width, int height)
{
	struct color_buffer tmp;
	glGenFramebuffers(1, &tmp.fbo);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, tmp.fbo);

	glGenTextures(1, &tmp.texture);
	glGenTextures(1, &tmp.depth);
	
	glBindTexture(GL_TEXTURE_2D, tmp.texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tmp.texture, 0);

	glBindTexture(GL_TEXTURE_2D, tmp.depth);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, tmp.depth, 0);

	glDrawBuffer(GL_COLOR_ATTACHMENT0);

	GLenum error = glCheckFramebufferStatus(GL_FRAMEBUFFER);

	if (error != GL_FRAMEBUFFER_COMPLETE) {
		printf("FB error, status: 0x%x\n", error);
	}

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

	return tmp;
}

struct renderable_cubemap renderable_cubemap_new(int width)
{
	struct renderable_cubemap tmp;
	tmp.width = width;
	glGenFramebuffers(1, &tmp.fbo);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, tmp.fbo);

	glGenTextures(1, &tmp.texture);
	glGenTextures(1, &tmp.depth);

	glBindTexture(GL_TEXTURE_CUBE_MAP, tmp.texture);

	for (int i = 0; i < 6; i++)
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, tmp.width, tmp.width, 0, GL_RGB, GL_FLOAT, NULL);

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	//Bind positive x for now.
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X, tmp.texture, 0);

	glBindTexture(GL_TEXTURE_2D, tmp.depth);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, tmp.width, tmp.width, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, tmp.depth, 0);

	glDrawBuffer(GL_COLOR_ATTACHMENT0);

	GLenum error = glCheckFramebufferStatus(GL_FRAMEBUFFER);

	if (error != GL_FRAMEBUFFER_COMPLETE) {
		printf("FB error, status: 0x%x\n", error);
	}

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

	return tmp;
}

void delete_deferred_framebuffer(struct deferred_framebuffer fb)
{
	glDeleteTextures(GBUFFER_NUM_TEXTURES, fb.textures);
	glDeleteTextures(1, &fb.depth);
	glDeleteFramebuffers(1, &fb.fbo);
}

void delete_accumulation_buffer(struct accumulation_buffer ab)
{
	glDeleteTextures(LENGTH(ab.textures), ab.textures);
	glDeleteTextures(1, &ab.depth);
	glDeleteFramebuffers(1, &ab.fbo);
}

void color_buffer_delete(struct color_buffer cb)
{
	glDeleteTextures(1, &cb.texture);
	glDeleteTextures(1, &cb.depth);
	glDeleteFramebuffers(1, &cb.fbo);
}

void bind_deferred_for_reading(struct deferred_framebuffer fb, struct accumulation_buffer ab)
{
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, ab.fbo);
	for (int i = 0; i < LENGTH(fb.textures); i++) {
		glActiveTexture(GL_TEXTURE0 + i);	
		glBindTexture(GL_TEXTURE_2D, fb.textures[GBUFFER_TEXTURE_TYPE_POSITION + i]);
	}
	glBindFramebuffer(GL_READ_FRAMEBUFFER, fb.fbo);
}

void bind_accumulation_for_reading(struct accumulation_buffer ab)
{
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    for (int i = 0; i < LENGTH(ab.textures); i++) {
		glActiveTexture(GL_TEXTURE0 + i);	
		glBindTexture(GL_TEXTURE_2D, ab.textures[i]);
    }
	glBindFramebuffer(GL_READ_FRAMEBUFFER, ab.fbo);
}

void color_buffer_bind_for_reading(struct color_buffer cb)
{
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glActiveTexture(GL_TEXTURE0);	
	glBindTexture(GL_TEXTURE_2D, cb.texture);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, cb.fbo);
}