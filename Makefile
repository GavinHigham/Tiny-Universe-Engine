CC = gcc
SDL = -framework SDL2 -framework SDL2_image -framework OpenGL -lGLEW 
CFLAGS = -Wall -c -std=c99 -g -O3 -pthread
LDFLAGS = $(SDL)
OBJECTS = main.o init.o image_load.o global_images.o keyboard.o render.o shaders.o \
affine_matrix4.o matrix3.o vector3.o models.o buffer_group.o controller.o deferred_framebuffer.o \
lights.o func_list.o shader_utils.o gl_utils.o procedural_terrain.o
SHADERS = shaders/*
MODELS = models/*
SHADER_GENERATORS = generate_shader_source.awk generate_shader_header.awk
EXE = sock

all: $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $(EXE)

init.o: shaders.h

render.o: shaders.h models.h

models.c: $(MODELS) generate_model_source.awk models.h
	awk -f generate_model_source.awk $(MODELS) > models.c

models.h: $(MODELS) generate_model_header.awk
	awk -f generate_model_header.awk $(MODELS) > models.h

shaders.c: $(SHADERS) $(SHADER_GENERATORS) shaders.h
	awk -f generate_shader_source.awk $(SHADERS) > shaders.c

shaders.h: $(SHADERS) $(SHADER_GENERATORS)
	awk -f generate_shader_header.awk $(SHADERS) > shaders.h

buffer_group.h: shaders.h

buffer_group.c: buffer_group.h

clean:
	rm $(OBJECTS) && rm $(EXE)