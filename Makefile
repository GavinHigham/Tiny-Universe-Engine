CC = gcc
SDL 	 = #-framework SDL2 -framework SDL2_image -framework OpenGL -lGLEW
INCLUDES = -Iglla -I$(CURDIR) \
-IC:\MinGW\include \
-IC:\Users\Gavin\Code\_libs_\lua-5.3.5\src \
-IC:\Users\Gavin\Code\_libs_\SDL2\SDL2-2.0.8\x86_64-w64-mingw32\include \
-IC:\Users\Gavin\Code\_libs_\glew-2.1.0-win32\glew-2.1.0\include \
-I$(CURDIR)\glad\include


ifeq ($(CC),gcc)
	libs=$(libs_for_gcc)
else
	libs=$(normal_libs)
endif

LDFLAGS	 = $(SDL) \
-LC:\Users\Gavin\Code\_libs_\SDL2_image-2.0.4\x86_64-w64-mingw32\lib \
-LC:\Users\Gavin\Code\_libs_\SDL2\SDL2-2.0.8\x86_64-w64-mingw32\lib \
-LC:\ProgramData\chocolatey\lib\mingw\tools\install\mingw64\lib\gcc\x86_64-w64-mingw32\8.1.0 \
-LC:\ProgramData\chocolatey\lib\mingw\tools\install\mingw64\x86_64-w64-mingw32\lib \
-LC:\MinGW\lib \
# -LC:\Users\Gavin\Code\_libs_\glew-2.1.0-win32\glew-2.1.0\lib\Release\x64 \
# -lglew32s.lib \
# -llua \
# -target x86_64-pc-windows-gnu
SHADERS	 = $(wildcard shaders/*.vs shaders/*.fs shaders/*.gs)
LUA      = $(patsubst %.c,%.o,$(wildcard lua53/*.c))
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

CFLAGS 	= -Wall -c -std=c11 -g \
$(INCLUDES) \
# -target x86_64-pc-windows-gnu \
# -s USE_SDL=2 \
# -s USE_SDL_IMAGE=2 \
# -s SDL2_IMAGE_FORMATS='["png"]' \
#-march=native -O3

all: $(OBJECTS) #ceffectpp/ceffectpp
	$(CC) $(LDFLAGS) $(OBJECTS) -o $(EXE)

#I want "all" to be the default rule, so any module-specific build rules should be specified in a separate makefile.
include models/Makefile

.depend: #Generate dependencies from all .c files, searching recursively.
	gcc -M $(INCLUDES) $(**/.c) *.c > .depend

effects.c: $(SHADERS) effects.h #ceffectpp/ceffectpp
	"ceffectpp/ceffectpp" -c $(SHADERS) > effects.c

effects.h: $(SHADERS) #ceffectpp/ceffectpp
	"ceffectpp/ceffectpp" -h $(SHADERS) > effects.h

ceffectpp/ceffectpp:
	cd ceffectpp && make

open-simplex-noise.o:
	cd open-simplex-noise-in-c && make

clean:
	rm $(OBJECTS)
	rm $(EXE)
	rm .depend
	cd ceffectpp; make clean
	cd open-simplex-noise-in-c; make clean

include .depend
