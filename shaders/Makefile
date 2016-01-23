CC = gcc
SDL = -framework OpenGL -lGLEW 
CFLAGS = -Wall -c -std=c99 -g -O3
OBJECTS = shaders.o
SHADERS = programs/*
SHADER_GENERATORS = generate_shader_source.awk generate_shader_header.awk

shaders.c: $(SHADERS) $(SHADER_GENERATORS) shaders.h
	awk -f generate_shader_source.awk $(SHADERS) > shaders.c

shaders.h: $(SHADERS) $(SHADER_GENERATORS)
	awk -f generate_shader_header.awk $(SHADERS) > shaders.h

clean:
	rm $(OBJECTS)