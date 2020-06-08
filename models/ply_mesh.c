#include "models/ply_mesh.h"
#include "math/utility.h"

extern lua_State *L;

void ply_mesh_free(struct ply_mesh *mesh)
{
	free(mesh);
}

struct ply_mesh * ply_mesh_load(const char *filename)
{
	//Push required args for Lua ParsePLY.parseFile
	int top = lua_gettop(L);

	if (lua_getglobal(L, "PlyParser") != LUA_TTABLE) {
		printf("Global \"PlyParser\" is not a table!\n");
		lua_settop(L, top);
		return NULL;
	}
	lua_getfield(L, -1, "parseFile");
	lua_pushstring(L, filename);

	if (lua_pcall(L, 1, 1, 0) != LUA_OK) {
		luaconf_error(L, "Cannot parse %s, an error occurred: %s", filename, lua_tostring(L, -1));
		lua_settop(L, top);
		return NULL;
	}

	//At this point, the table (or nil) is on the stack
	char *error = "(result is nil)";
	switch(lua_type(L, -1)) {
	case LUA_TSTRING:
		error = lua_tostring(L, -1);
		//fall-through
	case LUA_TNIL:
		luaconf_error(L, "Cannot parse %s, an error occurred: %s", filename, error);
		lua_settop(L, top);
		return NULL;
	case LUA_TTABLE:
		break;
	default:
		return NULL;
	}



	size_t total_size;
	size_t remaining = total_size;
	void *slab = malloc(total_size);
	struct ply_mesh *mesh = alloc_from_chunk(slab, &remaining, sizeof(struct ply_mesh));
	//TODO get filename_len
	mesh->filename = alloc_from_chunk(slab, &remaining, filename_len);
	//TODO get mesh element count
	mesh->elements = alloc_from_chunk(slab, &remaining, mesh->count * sizeof(struct ply_element));

	for (int i = 0; i < mesh->count; i++) {
		struct ply_element *element = mesh->elements[i];
		//TODO set element count
		//TODO get element_name_len
		element->name = alloc_from_chunk(slab, &remaining, element_name_len);
		for (int j = 0; j < element->count; j++) {
			struct ply_property *property = element->properties[i];
			//TODO set property type
			//TODO set property list_type
			//TODO set property count_type
			//TODO get property name_len
			property->name = alloc_from_chunk(slab, &remaining, name_len);
		}
		//TODO fill data for each element
	}
}