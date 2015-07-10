CC = clang
SDL = -framework SDL2 -framework SDL2_image
CFLAGS = -Wall -c -std=c99 -g
LDFLAGS = $(SDL)
OBJECTS = main.o init.o image_load.o global_images.o keyboard.o
EXE = main

all: $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $(EXE)

clean:
	rm $(OBJECTS) && rm $(EXE)