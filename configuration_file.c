#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "configuration_file.h"

//Parse config file. Will need a mechanism to register configurable variables.
//When the config file is parsed, they will be set.
//Registering should provide the attribute tag (user-exposed name), and a pointer to the variable which is to be set.

struct config_buf config_new(char *tag_buf, int tag_buf_len, struct config_var *var_buf, int var_buf_len)
{
	struct config_buf tmp = {
		.tag_buf = tag_buf,
		.var_buf = var_buf,
		.tag_buf_len = tag_buf_len,
		.var_buf_len = var_buf_len,
		.var_buf_used_len = 0,
		.tag_buf_used_len = 0
	};
	return tmp;
}

int config_expose(struct config_buf *cb, void *ptr, char *tag_name, enum config_var_type type)
{
	if (cb->var_buf_used_len < cb->var_buf_len && (strlen(tag_name) + 1) <= (cb->tag_buf_len - cb->tag_buf_used_len)) {
		struct config_var tmp = {
			.type = type,
			.var_ptr = ptr,
			.tag = &(cb->tag_buf[cb->tag_buf_used_len])
		};
		cb->var_buf[cb->var_buf_used_len] = tmp;
		cb->var_buf_used_len++;
		cb->tag_buf_used_len += sprintf(&cb->tag_buf[cb->tag_buf_used_len], "%s", tag_name) + 1;
		return 0;
	}
	return 1;
}


int config_expose_int(struct config_buf *cb, int *ptr, char *tag_name)
{
	return config_expose(cb, (void *)ptr, tag_name, _CONFIG_INT);
}

int config_expose_float(struct config_buf *cb, float *ptr, char *tag_name)
{
	return config_expose(cb, (void *)ptr, tag_name, _CONFIG_FLOAT);
}

int config_expose_float3(struct config_buf *cb, float *ptr, char *tag_name)
{
	return config_expose(cb, (void *)ptr, tag_name, _CONFIG_FLOAT3);
}

int config_load(struct config_buf cb, char *file_path)
{
	FILE * fp;
	char * line = NULL;
	size_t len = 0;
	ssize_t read;

	fp = fopen(file_path, "r");
	if (fp == NULL)
		return 1;

	while ((read = getline(&line, &len, fp)) != -1) {
		printf("%s", line);
		char *value = strchr(line, ':');
		if (value) {
			*value = '\0';
			value++;
			for (int i = 0; i < cb.var_buf_used_len; i++) {
				if (strcmp(line, cb.var_buf[i].tag) == 0) {
					struct config_var var = cb.var_buf[i];
					//char *value = strtok(NULL, delimiters);
					float *fptr = ((float *)var.var_ptr);
					switch (var.type) {
					case _CONFIG_INT:
						sscanf(value, "%i", (int *)var.var_ptr);
						break;
					case _CONFIG_FLOAT:
						sscanf(value, "%f", fptr);
						break;
					case _CONFIG_FLOAT3:
						sscanf(value, "%f %f %f", fptr, fptr + 1, fptr + 2);
						break;
					case _CONFIG_STRING:
						assert(0); //Not yet implemented.
						break;
					default:
						break;
					}
				}
			}
		}
	}

	fclose(fp);
	if (line)
		free(line);
	return 0;
}
