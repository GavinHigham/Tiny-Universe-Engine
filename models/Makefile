CC = gcc
SDL = -framework OpenGL -lGLEW 
CFLAGS = -Wall -c -std=c99 -g -O3
OBJECTS = models.o 
MODELS = source_models/*

models.c: $(MODELS) generate_model_source.awk models.h
	awk -f generate_model_source.awk $(MODELS) > models.c

models.h: $(MODELS) generate_model_header.awk
	awk -f generate_model_header.awk $(MODELS) > models.h

clean:
	rm $(OBJECTS)