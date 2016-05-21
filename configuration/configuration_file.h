#ifndef CONFIGURATION_FILE_H
#define CONFIGURATION_FILE_H
#include <stdbool.h>
#include "../math/vector3.h"

#define config_expose(cb, ptr, tag) config_expose_generic( _Generic((ptr), \
	int   *: _CONFIG_INT,    \
	float *: _CONFIG_FLOAT,  \
	vec3  *: _CONFIG_FLOAT3, \
	bool  *: _CONFIG_BOOL,   \
	default: _CONFIG_UNSUPPORTED) , cb, ptr, tag)

enum config_var_type {_CONFIG_INT, _CONFIG_FLOAT, _CONFIG_FLOAT3, _CONFIG_BOOL, _CONFIG_STRING, _CONFIG_UNSUPPORTED};

struct config_var {
	enum config_var_type type;
	void *var_ptr;
	char *tag;
};

struct config_buf {
	char *tag_buf;
	struct config_var *var_buf;
	int tag_buf_len;
	int var_buf_len;
	int var_buf_used_len;
	int tag_buf_used_len;
};

struct config_buf config_new(char *tag_buf, int tag_buf_len, struct config_var *var_buf, int var_buf_len);
int config_expose_generic(enum config_var_type type, struct config_buf *cb, void *ptr, char *tag_name);
int config_load(struct config_buf cb, char *file_path);

#endif