//AUTO-GENERATED FILE, CHANGES MAY BE OVERWRITTEN.

#ifndef EFFECTS_H
#define EFFECTS_H

#include "graphics.h"
typedef struct effect_data {
	GLuint handle;
	union {
		struct {
		};
		GLint unif[0];
	};
	union {
		struct {
		};
		GLint attr[0];
	};
} EFFECT;

union effect_list {
	struct {
	};
	EFFECT all[0];
};

union effect_list effects;

const char *uniform_strings[0];
const char *attribute_strings[0];
const char *shader_file_paths[0];

#endif
