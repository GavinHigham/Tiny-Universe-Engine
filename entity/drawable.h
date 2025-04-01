#ifndef DRAWABLE_H
#define DRAWABLE_H

#include <stdbool.h>
#include "glla.h"
#include "../graphics.h"
#include "../buffer_group.h"
#include "math/bpos.h"

/*
A drawable component should encapsulate the following:
What shader to use
What stage the entity should be drawn in
Whether the entity casts and receives shadows, and from which shadow groups
If the entity has a mesh component, or renders as some kind of primitive
	(Do I want refcounting on the meshes? I could use talloc for that.)
	Vertex format for the mesh
Any kind of custom drawing the drawable needs
What texture setup the drawable needs

I should also have some sort of batch drawing functions, to avoid unnessesary OpenGL state changes


Update 2019/02/07
Drawable components should have some form of sort key including a "batch ID", so that they can be sorted
and then batched by shader / textures / buffers / transparent-or-not / other pre-or-post condition.
One-off "only draw one of these" entities like current solar system, nearby stars, galaxy cubemap can just use their
own batch ID. Batch IDs could even be generated to some extent, I'd need to describe the pre-and-post OpenGL state for each
drawable, and then form some kind of minimal-state-change ordering based on that.
*/

typedef struct drawable_component {
	EFFECT *effect;
	amat4 *frame;
	bpos_origin *sector;
	struct buffer_group *bg;
	void (*draw)(EFFECT *, struct buffer_group, amat4);
	bool bg_from_malloc; //Until I clean up buffer_group, check this to decide if free() should be called on bg within deinit.
} Drawable;

typedef void (*Draw_func)(EFFECT *, struct buffer_group, amat4);

void init_drawable(Drawable *d, void (*draw)(EFFECT *, struct buffer_group, amat4), EFFECT *effect, amat4 *frame, bpos_origin *sector, struct buffer_group *buffers);
//Temporary Drawable init until I clean up buffer_group stuff.
void init_heap_drawable(Drawable *d, Draw_func draw, EFFECT *effect, amat4 *frame, bpos_origin *sector, int (*buffering_function)(struct buffer_group));
void deinit_drawable(Drawable *drawable);
void draw_drawable(Drawable *d);

#endif