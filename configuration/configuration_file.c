#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include "configuration_file.h"
#include <glalgebra.h>

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

int config_expose_generic(enum config_var_type type, struct config_buf *cb, void *ptr, char *tag_name)
{
	printf("Exposing %s\n", tag_name);
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

int config_expose_multiple(struct config_buf *cb, void **ptrs, char **tag_names, int len, enum config_var_type type)
{
		for (int i = 0; i < len; i++) {
		int error = config_expose_generic(type, cb, ptrs[i], tag_names[i]);
		if (error)
			return error;
	}
	return 0;
}

int config_expose_ints(struct config_buf *cb, int **ptrs, char **tag_names, int len)
{
	return config_expose_multiple(cb, (void **)ptrs, tag_names, len, _CONFIG_INT);
}

int config_expose_floats(struct config_buf *cb, float **ptrs, char **tag_names, int len)
{
	return config_expose_multiple(cb, (void **)ptrs, tag_names, len, _CONFIG_FLOAT);
}

int config_expose_float3s(struct config_buf *cb, float **ptrs, char **tag_names, int len)
{
	return config_expose_multiple(cb, (void **)ptrs, tag_names, len, _CONFIG_FLOAT3);
}

int config_expose_bools(struct config_buf *cb, bool **ptrs, char **tag_names, int len)
{
	return config_expose_multiple(cb, (void **)ptrs, tag_names, len, _CONFIG_BOOL);
}

void config_parse_set_value(struct config_var var, char *value)
{
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
	case _CONFIG_BOOL:
		{
			char *bstr = strtok(value, " \t\n"); //get rid of whitespace
			if (bstr) {
				//"true" and "false" are the only valid bool values
				if (!strcmp("true", bstr))
					*((int *)var.var_ptr) = 1;
				else if (!strcmp("false", bstr))
					*((int *)var.var_ptr) = 0;
			}
			printf("bool string: %s\n", bstr);
		}
		break;
	case _CONFIG_STRING:
		assert(0); //Not yet implemented.
		break;
	default:
		break;
	}	
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
		char *value = strchr(line, ':'); //Try to find the colon
		if (value) { //if we found the colon,
			*value = '\0'; //turn it into a \0 to terminate the tag string
			value++; //then advance the pointer to look at the value, if there is one
			for (int i = 0; i < cb.var_buf_used_len; i++) { //loop through the tags we recognize
				if (strcmp(line, cb.var_buf[i].tag) == 0) { //if there's a match, do the thing
					config_parse_set_value(cb.var_buf[i], value);
				}
			}
		}
	}

	fclose(fp);
	if (line)
		free(line);
	return 0;
}
