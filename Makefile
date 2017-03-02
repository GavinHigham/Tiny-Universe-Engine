CC = gcc
SDL = -framework SDL2 -framework SDL2_image -framework OpenGL -lGLEW
MODULE_PATHS = -I./glla
CFLAGS = $(MODULE_PATHS) -Wall -c -std=c11 -g -pthread
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
	procedural_terrain.o \
	effects.o \
	drawf.o \
	draw.o \
	drawable.o \
	terrain_erosion.o \
	triangular_terrain_tile.o \
	procedural_planet.o \
	ship_control.o \
	dynamic_terrain_tree.o \
	open-simplex-noise-in-c/open-simplex-noise.o \
	glla/glla.o

#Module includes append to OBJECTS and define other custom rules.
include configuration/configuration.mk
include math/math.mk
include models/models.mk

all: $(OBJECTS) open-simplex-noise
	$(CC) $(LDFLAGS) $(OBJECTS) -o $(EXE)

#I want "all" to be the default rule, so any module-specific build rules should be specified in a separate makefile.
include models/Makefile

#include entity_component.mk #include when I get around to making this

.depend:
	gcc -M $(**/.c) *.c > .depend #Generate dependencies from all .c files, searching recursively.

effects.c: ceffectpp $(SHADERS) effects.h
	ceffectpp/ceffectpp -c $(SHADERS) > effects.c

effects.h: ceffectpp $(SHADERS)
	ceffectpp/ceffectpp -h $(SHADERS) > effects.h

ceffectpp:
	cd ceffectpp; make

open-simplex-noise:
	cd open-simplex-noise-in-c; make

renderer.o: effects.o procedural_terrain.h

clean:
	rm $(OBJECTS) && rm $(EXE) && rm .depend

include .depend
