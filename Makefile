CC 	= gcc
SDL 	= -framework SDL2 -framework SDL2_image -framework OpenGL -lGLEW
CFLAGS 	= -Wall -c -std=c11 -g -pthread -Iglla -march=native -O0
LDFLAGS	= $(SDL) -llua
SHADERS	= shaders/*.vs shaders/*.fs shaders/*.gs
EXE 	= sock

#Module includes append to OBJECTS and define other custom rules.
include root.mk
include configuration/configuration.mk
include math/math.mk
include models/models.mk
include entity/entity.mk
include glsw/glsw.mk
include space/space.mk
include experiments/experiments.mk

all: $(OBJECTS) #ceffectpp/ceffectpp
	$(CC) $(LDFLAGS) $(OBJECTS) -o $(EXE)

#I want "all" to be the default rule, so any module-specific build rules should be specified in a separate makefile.
include models/Makefile

.depend:
	gcc -M -Iglla $(**/.c) *.c > .depend #Generate dependencies from all .c files, searching recursively.

effects.c: $(SHADERS) effects.h #ceffectpp/ceffectpp
	ceffectpp/ceffectpp -c $(SHADERS) > effects.c

effects.h: $(SHADERS) #ceffectpp/ceffectpp
	ceffectpp/ceffectpp -h $(SHADERS) > effects.h

# ceffectpp/ceffectpp:
# 	cd ceffectpp; make

open-simplex-noise.o:
	cd open-simplex-noise-in-c; make

clean:
	rm $(OBJECTS)
	rm $(EXE)
	rm .depend
	cd ceffectpp; make clean
	cd open-simplex-noise-in-c; make clean

include .depend
