CC = gcc
SDL = -framework SDL2 -framework SDL2_image -framework OpenGL -lGLEW
CFLAGS = -Wall -c -std=c99 -g -pthread
LDFLAGS = $(SDL) -lglla
MATH_OBJECTS = math/utility.o
MODELS_OBJECTS = models/models.o
SHADERS = shaders/*.vs shaders/*.fs shaders/*.gs
SHADER_GENERATOR = /usr/local/bin/ceffectpp
CONFIGURATION_OBJECTS = configuration/configuration_file.o
OBJECTS = main.o init.o image_load.o keyboard.o renderer.o buffer_group.o controller.o \
deferred_framebuffer.o lights.o func_list.o shader_utils.o gl_utils.o stars.o procedural_terrain.o \
effects.o drawf.o draw.o drawable.o terrain_erosion.o triangular_terrain_tile.o procedural_planet.o \
ship_control.o dynamic_terrain_tree.o open-simplex-noise-in-c/open-simplex-noise.o \
$(MATH_OBJECTS) $(MODELS_OBJECTS) $(CONFIGURATION_OBJECTS)
EXE = sock

all: $(OBJECTS) math_module models_module configuration_module open-simplex-noise
	$(CC) $(LDFLAGS) $(OBJECTS) -o $(EXE)

.depend:
	gcc -M *.c > .depend

init.o: effects.o

math_module:
	cd math; make

models_module:
	cd models; make

# shaders_module:
# 	cd shaders; make

effects.c: $(SHADERS) $(SHADER_GENERATOR) effects.h
	$(SHADER_GENERATOR) -c $(SHADERS) > effects.c

effects.h: $(SHADERS) $(SHADER_GENERATOR)
	$(SHADER_GENERATOR) -h $(SHADERS) > effects.h

configuration_module:
	cd configuration; make

open-simplex-noise:
	cd open-simplex-noise-in-c; make

renderer.o: effects.o procedural_terrain.h models_module

buffer_group.h: effects.o

buffer_group.c: buffer_group.h

dynamic_terrain.o: dynamic_terrain.h procedural_terrain.h triangular_terrain_tile.h terrain_constants.h terrain_types.h

clean:
	rm $(OBJECTS) && rm $(EXE)
	rm .depend

rclean: clean
	cd math; make clean
	cd models; make clean

include .depend
