CC = gcc
CFLAGS = -Wall -c -std=c11 -O3
OBJECTS = ceffectpp.o table.o print_effect.o
EXE = ceffectpp

all: $(OBJECTS)
	$(CC) $(OBJECTS) -o $(EXE)

install: all
	cp $(EXE) /usr/local/bin/

clean:
	rm $(OBJECTS) $(EXE)