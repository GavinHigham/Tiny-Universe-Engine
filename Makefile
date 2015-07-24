CC = clang
SDL = -framework SDL2 -framework SDL2_image -framework OpenGL -lGLEW
CFLAGS = -Wall -c -std=c99 -g
LDFLAGS = $(SDL)
OBJECTS = main.o init.o image_load.o global_images.o keyboard.o render.o shaders.o
SHADERS = shaders/simple.vs shaders/simple.fs
SHADER_GENERATORS = generate_shader_source.awk generate_shader_header.awk
EXE = main

all: $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $(EXE)

shaders.o: shaders

shaders: $(SHADERS) $(SHADER_GENERATORS)
	awk -f generate_shader_header.awk $(SHADERS) > shaders.h
	awk -f generate_shader_source.awk $(SHADERS) > shaders.c

clean:
	rm $(OBJECTS) && rm $(EXE)