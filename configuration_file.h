#ifndef CONFIGURATION_FILE_H
#define CONFIGURATION_FILE_H

enum config_var_type {_CONFIG_INT, _CONFIG_FLOAT, _CONFIG_FLOAT3, _CONFIG_BOOL, _CONFIG_STRING};

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
int config_expose_int(struct config_buf *cb, int *ptr, char *tag_name);
int config_expose_float(struct config_buf *cb, float *ptr, char *tag_name);
int config_expose_float3(struct config_buf *cb, float *ptr, char *tag_name);
int config_expose_bool(struct config_buf *cb, int *ptr, char *tag_name);
int config_load(struct config_buf cb, char *file_path);

#endif