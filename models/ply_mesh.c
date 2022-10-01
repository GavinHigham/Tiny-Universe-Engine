#include "models/ply_mesh.h"
#include "math/utility.h"
#include <lua-5.4.4/src/lua.h>
#include <lua-5.4.4/src/lauxlib.h>
#include <stdbool.h>
#include <inttypes.h>

// #define ply_print(args...) printf(args)
#define ply_print(args...)

extern lua_State *L;

void ply_mesh_free(struct ply_mesh *mesh)
{
	free(mesh);
}

const char * ply_mesh_type_to_string(enum ply_property_type type)
{
	const char *typestrings[] = {
		"invalid",
		"char",
		"uchar",
		"short",
		"ushort",
		"int",
		"uint",
		"float",
		"double",
		"list"
	};

	if (type >= 0 && type < PLY_TYPE_MAX)
		return typestrings[type];
	return NULL;
}

enum ply_property_type ply_mesh_string_to_type(const char *typestring)
{
	ply_print("string_to_type(%s)\n", typestring ? typestring : "null");
	//TODO: Reorder these by the most common? (Use a trie?)
	if (!typestring)
		return PLY_TYPE_INVALID;
	else if (!strcmp(typestring, "char"))
		return PLY_TYPE_CHAR;
	else if (!strcmp(typestring, "uchar"))
		return PLY_TYPE_UCHAR;
	else if (!strcmp(typestring, "short"))
		return PLY_TYPE_SHORT;
	else if (!strcmp(typestring, "ushort"))
		return PLY_TYPE_USHORT;
	else if (!strcmp(typestring, "int"))
		return PLY_TYPE_INT;
	else if (!strcmp(typestring, "uint"))
		return PLY_TYPE_UINT;
	else if (!strcmp(typestring, "float"))
		return PLY_TYPE_FLOAT;
	else if (!strcmp(typestring, "double"))
		return PLY_TYPE_DOUBLE;
	else if (!strcmp(typestring, "list"))
		return PLY_TYPE_LIST;
	return PLY_TYPE_INVALID;
}

void ply_mesh_push_list(void **data, struct ply_property *property, lua_Number list_len);

void ply_mesh_push(void **data, struct ply_property *property, enum ply_property_type type, lua_Number d, bool do_list)
{
	#define PUSHVAL(ptr, type, val) *((type *)ptr) = (type)val; ptr = (type *)ptr+1;
	switch (type) {
	case PLY_TYPE_CHAR:
		PUSHVAL(*data, int8_t, d)
		break;
	case PLY_TYPE_UCHAR:
		PUSHVAL(*data, uint8_t, d)
		break;
	case PLY_TYPE_SHORT:
		PUSHVAL(*data, int16_t, d)
		break;
	case PLY_TYPE_USHORT:
		PUSHVAL(*data, uint16_t, d)
		break;
	case PLY_TYPE_INT:
		PUSHVAL(*data, int32_t, d)
		break;
	case PLY_TYPE_UINT:
		PUSHVAL(*data, uint32_t, d)
		break;
	case PLY_TYPE_FLOAT:
		PUSHVAL(*data, float, d)
		break;
	case PLY_TYPE_DOUBLE:
		PUSHVAL(*data, double, d)
		break;
	case PLY_TYPE_LIST:
		if (do_list)
			ply_mesh_push_list(data, property, d);
		break;
	case PLY_TYPE_INVALID:
	//Fall-through
	default:
		//TODO error-handling
		ply_print("Invalid value to push!\n");
	}
}

void ply_mesh_push_list(void **data, struct ply_property *property, lua_Number list_len)
{
	ply_print("Pushing list with count_type %s and item_type %s and length %i\n",
		ply_mesh_type_to_string(property->count_type),
		ply_mesh_type_to_string(property->item_type),
		(int)list_len);

	ply_mesh_push(data, property, property->count_type, list_len, false);
	for (int i = 0; i < list_len; i++) {
		ply_print("\tPushing item %i\n", i);
		lua_rawgeti(L, -2, i+1);
		lua_Number d = lua_tonumber(L, -1);
		ply_mesh_push(data, property, property->item_type, d, false);
		lua_pop(L, 1);
	}
}

void ply_mesh_push_data_value(void **data, struct ply_property *property)
{
	enum ply_property_type type = property->type;
	ply_print("Pushing property of type %s (%i)\n", ply_mesh_type_to_string(type), type);

	//Might be nice to get this conditional out of the hot path
	if (type == PLY_TYPE_LIST)
		lua_len(L, -1);

	lua_Number d = lua_tonumber(L, -1);
	ply_mesh_push(data, property, type, d, true);

}

//This whole function is way too big and way too unsafe, I should find a better way to do this.
struct ply_mesh * ply_mesh_load(const char *filename, int flags)
{
	//Push required args for Lua ParsePLY.parseFile
	int top = lua_gettop(L);

	if (lua_getglobal(L, "PlyParser") != LUA_TTABLE) {
		ply_print("Global \"PlyParser\" is not a table!\n");
		lua_settop(L, top);
		return NULL;
	}
	ply_print("About to get parseFile\n");

	lua_getfield(L, -1, "parseFile");
	lua_pushstring(L, filename);

	lua_pushboolean(L, flags & PLY_LOAD_GEN_IB);
	lua_pushboolean(L, flags & PLY_LOAD_GEN_AIB);
	lua_pushboolean(L, flags & PLY_LOAD_GEN_NORMALS);

	ply_print("About to make call\n");

	if (lua_pcall(L, 4, 1, 0) != LUA_OK) {
		ply_print("Cannot parse %s, an error occurred: %s\n", filename, lua_tostring(L, -1));
		lua_settop(L, top);
		return NULL;
	}

	//At this point, the table (or nil) is on the stack
	const char *error = "(result is nil)";
	switch(lua_type(L, -1)) {
	case LUA_TSTRING:
		error = lua_tostring(L, -1);
		//fall-through
	case LUA_TNIL:
		ply_print("Cannot parse %s, an error occurred: %s\n", filename, error);
		lua_settop(L, top);
		return NULL;
	case LUA_TTABLE:
		break;
	default:
		return NULL;
	}

	ply_print("Getting sizes\n");

	//Rewrite this section in a Lua function?
	size_t total_size = sizeof(struct ply_mesh);
	size_t filename_len = strlen(filename) + 1;
	total_size += filename_len;

	size_t num_elements = luaL_len(L, -1);
	total_size += num_elements * sizeof(struct ply_element);
	for (int i = 0; i < num_elements; i++) {
		ply_print("Getting element %i\n", i);
		lua_rawgeti(L, -1, i+1);
		lua_getfield(L, -1, "properties");
		size_t num_properties = luaL_len(L, -1);
		total_size += num_properties * sizeof(struct ply_property);
		for (int j = 0; j < num_properties; j++) {
			ply_print("Getting property %i of element %i\n", j, i);
			lua_rawgeti(L, -1, j+1);
			lua_getfield(L, -1, "name");
			total_size += luaL_len(L, -1) + 1;
			lua_pop(L, 2);
		}

		lua_getfield(L, -2, "data_size");
		total_size += lua_tointeger(L, -1);
		lua_getfield(L, -3, "name");
		total_size += luaL_len(L, -1) + 1;
		lua_pop(L, 4);
	}

	ply_print("Got sizes\n");

	int newtop = lua_gettop(L);
	ply_print("top, newtop = %i, %i\n", top, newtop);
	if (newtop == top + 2)
		ply_print("Top is good!\n");
	else
		ply_print("Top is bad!\n");

	// total_size *= 2; //For testing

	size_t remaining = total_size;
	void *slab = malloc(total_size);
	struct ply_mesh *mesh = alloc_from_chunk(&slab, &remaining, sizeof(struct ply_mesh));
	mesh->filename = alloc_from_chunk(&slab, &remaining, filename_len);
	memcpy(mesh->filename, filename, filename_len);
	mesh->num_elements = num_elements;
	mesh->elements = alloc_from_chunk(&slab, &remaining, mesh->num_elements * sizeof(struct ply_element));

	//For each element
	for (int i = 0; i < mesh->num_elements; i++) {
		ply_print("Getting element %i/%lu, top: %i\n", i+1, mesh->num_elements, lua_gettop(L));
		struct ply_element *element = &(mesh->elements[i]);
		lua_rawgeti(L, top+2, i+1);

		ply_print("Getting properties\n");
		lua_getfield(L, top+3, "properties");
		element->num_properties = luaL_len(L, -1);
		element->properties = alloc_from_chunk(&slab, &remaining, sizeof(struct ply_property) * element->num_properties);
		// lua_pop(L, 1);

		ply_print("Getting name\n");
		size_t element_name_len;
		lua_getfield(L, top+3, "name");
		const char *element_name = lua_tolstring(L, -1, &element_name_len);
		element_name_len++;
		element->name = alloc_from_chunk(&slab, &remaining, element_name_len);
		memcpy(element->name, element_name, element_name_len);

		ply_print("Getting count\n");
		lua_getfield(L, top+3, "count");
		element->count = lua_tointeger(L, -1);

		ply_print("top: %i\n", lua_gettop(L));

		//For each property
		for (int j = 0; j < element->num_properties; j++) {
			ply_print("Getting property %i/%lu of element %i, top: %i\n", j+1, element->num_properties, i, lua_gettop(L));
			struct ply_property *property = &(element->properties[j]);
			lua_rawgeti(L, top+4, j+1);
			size_t property_name_len;
			lua_getfield(L, -1, "name");
			ply_print("Getting name\n");
			const char *property_name = lua_tolstring(L, -1, &property_name_len);
			ply_print("%s\n", property_name);
			property_name_len++;
			property->name = alloc_from_chunk(&slab, &remaining, property_name_len);
			memcpy(property->name, property_name, property_name_len);

			ply_print("Getting type\n");
			lua_getfield(L, -2, "type");
			property->type = ply_mesh_string_to_type(lua_tostring(L, -1));

			if (property->type == PLY_TYPE_LIST) {
				ply_print("type is list\n");
				lua_getfield(L, -3, "item_type");
				property->item_type = ply_mesh_string_to_type(lua_tostring(L, -1));
				lua_getfield(L, -4, "count_type");
				property->count_type = ply_mesh_string_to_type(lua_tostring(L, -1));
				lua_pop(L, 2);
			} else {
				ply_print("type is not list\n");
				property->item_type = PLY_TYPE_INVALID;
				property->count_type = PLY_TYPE_INVALID;
			}

			lua_pop(L, 3);
		}

		lua_getfield(L, top+3, "data_size");
		element->data = alloc_from_chunk(&slab, &remaining, lua_tointeger(L, -1));
		lua_getfield(L, top+3, "data");
		size_t data_count = luaL_len(L, -1);

		ply_print("top: %i\n", lua_gettop(L));
		// size_t count = lua_tointeger(L, -1);

		void *dp = element->data;
		int data_top = lua_gettop(L);
		for (int j = 0; j < data_count; j++) {
			struct ply_property *property = &(element->properties[j%element->num_properties]);
			lua_rawgeti(L, -1, j+1);
			ply_mesh_push_data_value(&dp, property);
			lua_settop(L, data_top);
		}
		lua_pop(L, 6);
	}

	lua_settop(L, top);
	ply_print("total_size: %lu, remaining: %lu\n", total_size, remaining);
	return mesh;
}

void * ply_mesh_print_list(void *data, struct ply_property *property);

void * ply_mesh_print_data_value(void *data, struct ply_property *property, enum ply_property_type type, bool do_list)
{
	switch (type) {
	case PLY_TYPE_CHAR: {
		int8_t *ptr = data;
		ply_print("%i", *ptr);
		return ++ptr;
	}
	case PLY_TYPE_UCHAR: {
		uint8_t *ptr = data;
		ply_print("%u", *ptr);
		return ++ptr;
	}
	case PLY_TYPE_SHORT: {
		int16_t *ptr = data;
		ply_print("%i", *ptr);
		return ++ptr;
	}
	case PLY_TYPE_USHORT: {
		uint16_t *ptr = data;
		ply_print("%u", *ptr);
		return ++ptr;
	}
	case PLY_TYPE_INT: {
		int32_t *ptr = data;
		ply_print("%i", *ptr);
		return ++ptr;
	}
	case PLY_TYPE_UINT: {
		uint32_t *ptr = data;
		ply_print("%u", *ptr);
		return ++ptr;
	}
	case PLY_TYPE_FLOAT: {
		float *ptr = data;
		ply_print("%f", *ptr);
		return ++ptr;
	}
	case PLY_TYPE_DOUBLE: {
		double *ptr = data;
		ply_print("%f", *ptr);
		return ++ptr;
	}
	case PLY_TYPE_LIST:
		if (do_list)
			return ply_mesh_print_list(data, property);
	case PLY_TYPE_INVALID:
	//Fall-through
	default:
		ply_print("(invalid)");
	}
	return data;
}

double ply_mesh_get(void **data, enum ply_property_type type)
{
	switch (type) {
	case PLY_TYPE_CHAR: {
		int8_t *ptr = *data;
		*data = ptr+1;
		return *ptr;
	}
	case PLY_TYPE_UCHAR: {
		uint8_t *ptr = *data;
		*data = ptr+1;
		return *ptr;
	}
	case PLY_TYPE_SHORT: {
		int16_t *ptr = *data;
		*data = ptr+1;
		return *ptr;
	}
	case PLY_TYPE_USHORT: {
		uint16_t *ptr = *data;
		*data = ptr+1;
		return *ptr;
	}
	case PLY_TYPE_INT: {
		int32_t *ptr = *data;
		*data = ptr+1;
		return *ptr;
	}
	case PLY_TYPE_UINT: {
		uint32_t *ptr = *data;
		*data = ptr+1;
		return *ptr;
	}
	case PLY_TYPE_FLOAT: {
		float *ptr = *data;
		*data = ptr+1;
		return *ptr;
	}
	case PLY_TYPE_DOUBLE: {
		double *ptr = *data;
		*data = ptr+1;
		return *ptr;
	}
	case PLY_TYPE_INVALID:
	//Fall-through
	default:
		ply_print("Invalid value type, couldn't get\n");
	}
	return 0;
}

void * ply_mesh_print_list(void *data, struct ply_property *property)
{
	ply_mesh_print_data_value(data, property, property->count_type, false);
	double count = ply_mesh_get(&data, property->count_type); //Advances data pointer
	for (int i = 0; i < count; i++)
		ply_print(" %f", ply_mesh_get(&data, property->item_type));

	return data;
}

void ply_mesh_print(struct ply_mesh *mesh)
{
	ply_print("filename = '%s', num_elements = %lu\n", mesh->filename, mesh->num_elements);

	for (int i = 0; i < mesh->num_elements; i++) {
		struct ply_element *element = &(mesh->elements[i]);
		ply_print("\tElement %i, name = '%s', num_properties = %lu, count = %lu\n", i, element->name, element->num_properties, element->count);

		for (int j = 0; j < element->num_properties; j++) {
			struct ply_property *property = &(element->properties[j]);
			ply_print("\t\tProperty %i, name = '%s', type = '%s'", j, property->name, ply_mesh_type_to_string(property->type));

			if (property->type == PLY_TYPE_LIST)
				ply_print(" item_type = '%s', count_type = '%s'",
					ply_mesh_type_to_string(property->count_type),
					ply_mesh_type_to_string(property->item_type));
			ply_print("\n");
		}

		void *data = element->data;
		for (int j = 0; j < element->count; j++) {
			ply_print("\t\t\t");
			for (int k = 0; k < element->num_properties; k++) {
				struct ply_property *property = &(element->properties[k]);
				data = ply_mesh_print_data_value(data, property, property->type, true);
				ply_print(" ");
			}
			ply_print("\n");
		}
	}
}
