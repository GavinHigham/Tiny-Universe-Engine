CC 	= gcc
SDL 	 = -framework SDL2 -framework SDL2_image -framework OpenGL -lGLEW
INCLUDES = -Iglla -I$(CURDIR) -IC:\Users\Gavin\Code\_libs_\glew-2.1.0-win32\glew-2.1.0\include -IC:\Users\Gavin\Code\_libs_\SDL2\SDL2-2.0.8\x86_64-w64-mingw32\include -IC:\Users\Gavin\Code\_libs_\SDL2\SDL2-2.0.8\x86_64-w64-mingw32\include -IC:\Users\Gavin\Code\_libs_\lua-5.3.5\src
LDFLAGS	 = $(SDL) -llua -LC:\Users\Gavin\Code\_libs_\glew-2.1.0-win32\glew-2.1.0\lib\Release\x64 -LC:\Users\Gavin\Code\_libs_\SDL2_image-2.0.4\x86_64-w64-mingw32\lib -LC:\Users\Gavin\Code\_libs_\SDL2\SDL2-2.0.8\x86_64-w64-mingw32\lib -lglew32.lib
SHADERS	 = shaders/*.vs shaders/*.fs shaders/*.gs
EXE 	 = sock

#Module includes append to OBJECTS and define other custom rules.
include root.mk
include configuration/configuration.mk
include datastructures/datastructures.mk
include math/math.mk
include models/models.mk
include entity/entity.mk
include glsw/glsw.mk
include space/space.mk
include experiments/experiments.mk
include trackball/trackball.mk
include meter/meter.mk

CFLAGS 	= -Wall -c -std=c11 -g -pthread $(INCLUDES) #-march=native -O3
all: $(OBJECTS) #ceffectpp/ceffectpp
	$(CC) $(LDFLAGS) $(OBJECTS) -o $(EXE)

#I want "all" to be the default rule, so any module-specific build rules should be specified in a separate makefile.
include models/Makefile

.depend:
	gcc -M $(INCLUDES) $(**/.c) *.c > .depend #Generate dependencies from all .c files, searching recursively.

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
