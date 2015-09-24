#include <GL/glew.h>
#include "deferred_framebuffer.h"
#include "macros.h"

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

void delete_deferred_framebuffer(struct deferred_framebuffer fb)
{
	glDeleteTextures(GBUFFER_NUM_TEXTURES, fb.textures);
	glDeleteTextures(1, &fb.depth);
	glDeleteFramebuffers(1, &fb.fbo);
}

void bind_deferred_for_reading(struct deferred_framebuffer fb)
{
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    for (int i = 0; i < LENGTH(fb.textures); i++) {
		glActiveTexture(GL_TEXTURE0 + i);	
		glBindTexture(GL_TEXTURE_2D, fb.textures[GBUFFER_TEXTURE_TYPE_POSITION + i]);
    }
	glBindFramebuffer(GL_READ_FRAMEBUFFER, fb.fbo);
}