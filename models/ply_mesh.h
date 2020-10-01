#include <stdlib.h>

#ifndef PLY_MESH_H
#define PLY_MESH_H

//Definitions and functions for loading a representation of a PLY file
enum ply_property_type {
	PLY_TYPE_INVALID,
	PLY_TYPE_CHAR,
	PLY_TYPE_UCHAR,
	PLY_TYPE_SHORT,
	PLY_TYPE_USHORT,
	PLY_TYPE_INT,
	PLY_TYPE_UINT,
	PLY_TYPE_FLOAT,
	PLY_TYPE_DOUBLE,
	PLY_TYPE_LIST,
	PLY_TYPE_MAX
};

struct ply_property {
	enum ply_property_type type;
	enum ply_property_type item_type;
	enum ply_property_type count_type;
	char *name;
};

struct ply_element {
	char *name;
	size_t num_properties;
	size_t count;
	struct ply_property *properties;
	void *data;
};

struct ply_mesh {
	struct ply_element *elements;
	size_t num_elements;
	char *filename;
};

enum ply_load_flags {
	PLY_LOAD_GEN_IB = 1, //Generate index buffer as an additional element
	PLY_LOAD_GEN_AIB = 2, //Generate adjacency index buffer as an additional element
	PLY_LOAD_GEN_NORMALS = 4, //Generate normals as additional properties on the vertex element
};

struct ply_mesh * ply_mesh_load(const char *filename, int flags);
void ply_mesh_free(struct ply_mesh *mesh);
void ply_mesh_print(struct ply_mesh *mesh);

#endif