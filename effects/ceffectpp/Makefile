CC ?= gcc
CFLAGS = -Wall -c -std=c11 -O3
OBJECTS = ceffectpp.o table.o print_effect.o
EXE = ceffectpp

all: $(OBJECTS)
	$(CC) $(OBJECTS) -o $(EXE)

install: all
	cp $(EXE) /usr/local/bin/

CLEAN_CMD ?= rm -f
CLEAN_TARGETS := $(OBJECTS) $(EXE)

ifeq ($(OS),Windows_NT)
	EXE = ceffectpp.exe
	CLEAN_CMD := cmd /C del /F /Q
	CLEAN_TARGETS := $(subst /,\,$(CLEAN_TARGETS))
endif

.PHONY: clean
clean:
	-$(CLEAN_CMD) $(CLEAN_TARGETS)