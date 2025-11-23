CC ?= gcc

DEPEND_FILE := .depend
EXE = tu
LIB = libtu.so
PLATFORM_CFLAGS = -pthread -F/Library/Frameworks
LDFLAGS = $(SDL_LDFLAGS) -F/Library/Frameworks -rpath /Library/Frameworks -framework SDL3 -framework SDL3_image -framework OpenGL -Llua-5.4.4/src -llua
LIBFLAGS = $(SDL_LDFLAGS) $(LDFLAGS) -dynamiclib
LUA_TARGET = macosx
LUA_MAKEFLAGS =
CLEAN_CMD = rm -f
CEFFECTPP = effects/ceffectpp/ceffectpp
SDL_LIBS ?= -lSDL3_image -lSDL3

MAIN_OBJ = main.o
LIBTU_OBJ = libtu.o
CLEAN_TARGETS = $(MAIN_OBJ) $(OBJECTS) $(EXE) $(DEPEND_FILE)

# Windows build overrides
WINDOWS_BUILDS ?= $(filter Windows_NT,$(OS))
ifeq ($(WINDOWS_BUILDS),Windows_NT)
	CC = zig cc
	EXE = tu.exe
	LIB = libtu.dll
	PLATFORM_CFLAGS = -D_CRT_SECURE_NO_WARNINGS -DWIN32_LEAN_AND_MEAN -D_USE_MATH_DEFINES
	LDFLAGS  = -Llua-5.4.4/src -llua -lopengl32 $(SDL_LDFLAGS) $(SDL_LIBS)
	LIBFLAGS = -shared -Llua-5.4.4/src -llua -lopengl32 $(SDL_LDFLAGS) $(SDL_LIBS)
	LUA_TARGET = generic
	LUA_MAKEFLAGS = LUA_A=lua.lib CC="$(CC)" AR="zig ar --format=coff rcs" RANLIB="zig ranlib"
	CLEAN_CMD = cmd /C del /F /Q
	CEFFECTPP = effects\ceffectpp\ceffectpp.exe
	CLEAN_TARGETS := $(subst /,\,$(CLEAN_TARGETS))
endif

ifdef SDL3_DIR
	SDL_CFLAGS += -I$(subst \,/,$(SDL3_DIR))/include
	SDL_LDFLAGS += -L$(subst \,/,$(SDL3_DIR))/lib -L$(subst \,/,$(SDL3_DIR))/lib/x64
endif
ifdef SDL3_IMAGE_DIR
	SDL_CFLAGS += -I$(subst \,/,$(SDL3_IMAGE_DIR))/include
	SDL_LDFLAGS += -L$(subst \,/,$(SDL3_IMAGE_DIR))/lib -L$(subst \,/,$(SDL3_IMAGE_DIR))/lib/x64
endif

#Silly hack to make spaces in paths work
space := $(subst ,, )
CURDIR := $(subst $(space),\$(space),$(CURDIR))
INCLUDES = -Iglla -I$(CURDIR)
SHADERS	 = $(wildcard shaders/*.vs shaders/*.fs shaders/*.gs)
# LUA      = $(patsubst %.c,%.o,$(wildcard lua53/*.c)) #Should use this for everything except the experiments folder

#Module includes append to OBJECTS and INCLUDES and define other custom rules.
include root.mk
include datastructures/datastructures.mk
include math/math.mk
include models/models.mk
include entity/entity.mk
include glad/glad.mk
include glsw/glsw.mk
include space/space.mk
include experiments/experiments.mk
include trackball/trackball.mk
include meter/meter.mk
include components/components.mk
include systems/systems.mk
include luaengine/luaengine.mk
include effects/effects.mk
include test/test.mk

CFLAGS = -Wall -c -std=c23 -g $(INCLUDES) $(SDL_CFLAGS) $(PLATFORM_CFLAGS) #-march=native -O3
all: $(OBJECTS) $(MAIN_OBJ) $(LIBTU_OBJ) lua-5.4.4 #ceffectpp/ceffectpp
	$(CC) $(LDFLAGS) $(OBJECTS) $(MAIN_OBJ) -o $(EXE)
	$(CC) $(LIBFLAGS) $(OBJECTS) $(LIBTU_OBJ) -o $(LIB)


test_main.o: $(wildcard test/*.test.c)

test: $(OBJECTS) test_main.o Makefile
	./$(EXE) test

.PHONY: clean

# Generate dependencies from all .c files, searching recursively.
.depend:
	$(CC) -MM $(INCLUDES) $(filter-out -c,$(CFLAGS)) $(patsubst %.o,%.c,$(OBJECTS)) > $(DEPEND_FILE)

open-simplex-noise.o:
	cd open-simplex-noise-in-c; make

effects/effects.c: $(SHADERS) effects/effects.h lua-5.4.4 effects/ceffectpp/ceffectpp
	$(CEFFECTPP) -c $(SHADERS) > effects/effects.c

effects/effects.h: $(SHADERS) lua-5.4.4 effects/ceffectpp/ceffectpp
	$(CEFFECTPP) -h $(SHADERS) > effects/effects.h

effects/ceffectpp/ceffectpp:
	$(MAKE) -C effects/ceffectpp CC="$(CC)"

lua-5.4.4:
	$(MAKE) -C lua-5.4.4 $(LUA_TARGET) $(LUA_MAKEFLAGS)

clean:
	-$(CLEAN_CMD) $(CLEAN_TARGETS)
	$(MAKE) -C effects/ceffectpp clean
	$(MAKE) -C open-simplex-noise-in-c clean
	$(MAKE) -C lua-5.4.4 clean
