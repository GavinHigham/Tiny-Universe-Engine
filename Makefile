CC = clang
SDL = -framework SDL2 -framework SDL2_image -framework OpenGL -lGLEW
CFLAGS = -Wall -c -std=c99 -g
LDFLAGS = $(SDL)
OBJECTS = main.o init.o image_load.o global_images.o keyboard.o render.o shaders.o affine_matrix4.o
SHADERS = shaders/simple.vs shaders/simple.fs
SHADER_GENERATORS = generate_shader_source.awk generate_shader_header.awk
EXE = main

all: $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $(EXE)

init.o: shaders.h

render.o: shaders.h

shaders.c: $(SHADERS) $(SHADER_GENERATORS) shaders.h
	awk -f generate_shader_source.awk $(SHADERS) > shaders.c

shaders.h: $(SHADERS) $(SHADER_GENERATORS)
	awk -f generate_shader_header.awk $(SHADERS) > shaders.h

clean:
	rm $(OBJECTS) && rm $(EXE)