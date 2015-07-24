CC = clang
SDL = -framework SDL2 -framework SDL2_image -framework OpenGL -lGLEW
CFLAGS = -Wall -c -std=c99 -g
LDFLAGS = $(SDL)
OBJECTS = main.o init.o image_load.o global_images.o keyboard.o render.o shaders.o
EXE = main

all: $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $(EXE)

clean:
	rm $(OBJECTS) && rm $(EXE)