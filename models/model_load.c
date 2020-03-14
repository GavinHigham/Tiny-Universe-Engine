#include "graphics.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//This isn't actually used yet.

#define MAX_TOKENS_PER_LINE 20
#define VPROPERTY_POSITION_X 1 << 1
#define VPROPERTY_POSITION_Y 1 << 2
#define VPROPERTY_POSITION_Z 1 << 3
#define VPROPERTY_NORMAL_X   1 << 4
#define VPROPERTY_NORMAL_Y   1 << 5
#define VPROPERTY_NORMAL_Z   1 << 6
#define VPROPERTY_COLOR_R    1 << 7
#define VPROPERTY_COLOR_G    1 << 8
#define VPROPERTY_COLOR_B    1 << 9

//Add more vertex properties as powers of two here.

int property_flag_from_name(char *propname)
{
	int property_flags = 0;
	if (strcmp(propname, "x") == 0)
		property_flags |= VPROPERTY_POSITION_X;
	else if (strcmp(propname, "y") == 0)
		property_flags |= VPROPERTY_POSITION_Y;
	else if (strcmp(propname, "z") == 0)
		property_flags |= VPROPERTY_POSITION_Z;
	else if (strcmp(propname, "nx") == 0)
		property_flags |= VPROPERTY_NORMAL_X;
	else if (strcmp(propname, "ny") == 0)
		property_flags |= VPROPERTY_NORMAL_Y;
	else if (strcmp(propname, "nz") == 0)
		property_flags |= VPROPERTY_NORMAL_Z;
	else if (strcmp(propname, "red") == 0)
		property_flags |= VPROPERTY_COLOR_R;
	else if (strcmp(propname, "green") == 0)
		property_flags |= VPROPERTY_COLOR_G;
	else if (strcmp(propname, "blue") == 0)
		property_flags |= VPROPERTY_COLOR_B;
	return property_flags;
}

int numproperties(int property_flags)
{
	int num = 0;
	//Count number of bits set.
	//There's probably some freaky bitwise math way to do this in one instruction, but meh.
	while(property_flags) {
		num += property_flags & 1;
		property_flags >>= 1;
	}
	return num;
}

struct model_data {
	int property_flags;
	int vertex_bytes;
	int index_bytes;
};

struct model_data model_load(char *file_path, GLfloat *vertices, GLuint *indices)
{
	FILE * fp;
	char * line = NULL;
	size_t len = 0;
	ssize_t read;
	struct model_data data = {0};

	fp = fopen(file_path, "r");
	if (fp == NULL)
		return data;

/*
Steps are:
	Get number of vertices
	Figure out which properties are expressed in the file
	Get number of faces
	Buffer in vertices
	Buffer in faces as index buffer

	All the vertices can be loaded into a single buffer
	Just need to set attribute pointers, using a unique VAO for each object

*/
	int numverts = 0, numfaces = 0, numprops = 0;

	while ((read = getline(&line, &len, fp)) != -1) {
		char *tok = NULL;
		char *tokens[MAX_TOKENS_PER_LINE];
		//Tokenize each line.
		for (int i = 0; (tok = strtok(line, " ")) != NULL; i++) {
			tokens[i] = tok;
		}
		//Determine which properties are being represented in this vertex.
		if (strcmp(tokens[0], "property") == 0 && strcmp(tokens[2], "list") != 0) {
			numprops++;
			data.property_flags |= property_flag_from_name(tokens[3]);
		} else if (strcmp(tokens[0], "element") == 0) {
			if (strcmp(tokens[1], "vertex") == 0) {
				sscanf(tokens[2], "%i", &numverts); //Get the number of vertices.
			} else if (strcmp(tokens[1], "face") == 0) {
				sscanf(tokens[2], "%i", &numfaces); //Get the number of faces.
			}
		} else if (strcmp(tokens[0], "end_header") == 0) {
			break;
		}
	}
	if (numprops != numproperties(data.property_flags)) {
		printf("Unrecognized vertex format when reading model %s\n", file_path);
	} else {
		data.vertex_bytes = sizeof(GLfloat) * numprops * numverts;
		data.index_bytes = sizeof(GLuint) * 3 * numfaces;
	}
	//If we're here, we either hit the end of file, or we're done reading the header.
	while ((read = getline(&line, &len, fp)) != -1) {

		if (!(vertices && indices)) {
		} else {

		}
	}
	fclose(fp);
	if (line)
		free(line);
	return data;
}