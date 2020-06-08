#include <stdlib.h>

#ifndef PLY_MESH_H
#define PLY_MESH_H

enum ply_property_type {
	PLY_TYPE_CHAR,
	PLY_TYPE_UCHAR,
	PLY_TYPE_SHORT,
	PLY_TYPE_USHORT,
	PLY_TYPE_INT,
	PLY_TYPE_UINT,
	PLY_TYPE_FLOAT,
	PLY_TYPE_DOUBLE
};

struct ply_property {
	enum ply_property_type type;
	enum ply_property_type list_type;
	enum ply_property_type count_type;
	char *name;
};

struct ply_element {
	char *name;
	size_t count;
	struct ply_property *properties;
	void *data;
};

struct ply_mesh {
	struct ply_element *elements;
	size_t count;
	char *filename;
};

struct ply_mesh * ply_mesh_load(const char *);
void ply_mesh_free(struct ply_mesh *);

#endif