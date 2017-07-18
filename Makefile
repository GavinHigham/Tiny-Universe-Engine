CC = gcc
SDL = -framework SDL2 -framework SDL2_image -framework OpenGL -lGLEW
MODULE_PATHS = -Iglla
CFLAGS = $(MODULE_PATHS) -Wall -c -std=c11 -g -pthread -Iglla
LDFLAGS = $(SDL) -llua
SHADERS = shaders/*.vs shaders/*.fs shaders/*.gs
EXE = sock

#Objects in the top-level directory.
OBJECTS = \
	main.o \
	init.o \
	image_load.o \
	input_event.o \
	renderer.o \
	buffer_group.o \
	deferred_framebuffer.o \
	lights.o \
	shader_utils.o \
	gl_utils.o \
	stars.o \
	star_blocks.o \
	procedural_terrain.o \
	drawf.o \
	draw.o \
	terrain_erosion.o \
	triangular_terrain_tile.o \
	procedural_planet.o \
	open-simplex-noise-in-c/open-simplex-noise.o \
	effects.o \
	glla/glla.o \
	debug_graphics.o \
	quadtree.o

#Module includes append to OBJECTS and define other custom rules.
include configuration/configuration.mk
include math/math.mk
include models/models.mk
include entity/entity.mk

all: $(OBJECTS) ceffectpp/ceffectpp
	$(CC) $(LDFLAGS) $(OBJECTS) -o $(EXE)

#I want "all" to be the default rule, so any module-specific build rules should be specified in a separate makefile.
include models/Makefile

.depend:
	gcc -M -Iglla $(**/.c) *.c > .depend #Generate dependencies from all .c files, searching recursively.

effects.c: ceffectpp/ceffectpp $(SHADERS) effects.h
	ceffectpp/ceffectpp -c $(SHADERS) > effects.c

effects.h: ceffectpp/ceffectpp $(SHADERS)
	ceffectpp/ceffectpp -h $(SHADERS) > effects.h

ceffectpp/ceffectpp:
	cd ceffectpp; make

open-simplex-noise.o:
	cd open-simplex-noise-in-c; make

renderer.o: effects.o procedural_terrain.h

clean:
	rm $(OBJECTS)
	rm $(EXE)
	rm .depend
	cd ceffectpp; make clean
	cd open-simplex-noise-in-c; make clean

include .depend
