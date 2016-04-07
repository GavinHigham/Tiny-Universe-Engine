CC = gcc
SDL = -framework OpenGL -lGLEW 
CFLAGS = -Wall -c -std=c99 -g -O3
OBJECTS = models.o 
MODELS := $(wildcard source_models/*.ply)

models.o: models.c models.h

models.c: $(MODELS) generate_model_source.lua generate_model_source.awk models.h
	luajit generate_model_source.lua $(MODELS) > models.c

models.h: $(MODELS) generate_model_source.lua generate_model_source.awk generate_model_header.awk
	awk -f generate_model_header.awk $(MODELS) > models.h

clean:
	rm $(OBJECTS)