CC = gcc
SDL = -framework SDL2 -framework SDL2_image -framework OpenGL -lGLEW
CFLAGS = -Wall -c -std=c99 -g -pthread
LDFLAGS = $(SDL) -lglalgebra
MATH_OBJECTS = math/utility.o
MODELS_OBJECTS = models/models.o
SHADERS = shaders/*.vs shaders/*.fs shaders/*.gs
SHADER_GENERATOR = /usr/local/bin/ceffectpp
CONFIGURATION_OBJECTS = configuration/configuration_file.o
OBJECTS = main.o init.o image_load.o keyboard.o render.o buffer_group.o controller.o \
deferred_framebuffer.o lights.o func_list.o shader_utils.o gl_utils.o stars.o procedural_terrain.o \
effects.o \
open-simplex-noise-in-c/open-simplex-noise.o \
$(MATH_OBJECTS) $(MODELS_OBJECTS) $(CONFIGURATION_OBJECTS)
EXE = sock

all: $(OBJECTS) math_module models_module configuration_module open-simplex-noise
	$(CC) $(LDFLAGS) $(OBJECTS) -o $(EXE)

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

render.o: effects.o models_module

buffer_group.h: effects.o

buffer_group.c: buffer_group.h

clean:
	rm $(OBJECTS) && rm $(EXE)

rclean: clean
	cd math; make clean
	cd models; make clean
