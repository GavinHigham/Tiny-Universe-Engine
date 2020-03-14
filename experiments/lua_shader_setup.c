#include <lua-5.3.5/src/lua.h>
#include <lua-5.3.5/src/lauxlib.h>

#include "graphics.h"
#include "glsw_shaders.h"

int luaGetUniformHandleFromName(lua_State *L)
{
	int n = lua_gettop(L);
	lua_Number shader = 0;
	if (!lua_isnumber(L, 1) || !(shader = lua_tonumber(L, 1)) || !lua_isstring(L, 2))
		luaL_error(L, "Invalid arguments");

	const char *uniform_name = lua_tostring(L, 2);

	lua_Number handle = glGetUniformLocation(shader, uniform_name);
	lua_pushnumber(L, handle);
	return 1;
}

int luaGetAttributeHandleFromName(lua_State *L)
{
	int n = lua_gettop(L);
	lua_Number shader = 0;
	if (n < 2 || !lua_isnumber(L, 1) || !(shader = lua_tonumber(L, 1)) || !lua_isstring(L, 2))
		luaL_error(L, "Invalid arguments");

	const char *attribute_name = lua_tostring(L, 2);

	lua_Number handle = glGetAttribLocation(shader, attribute_name);
	lua_pushnumber(L, handle);
	return 1;
}

int luaGetShaderFromStrings(lua_State *L)
{
	int n = lua_gettop(L);
	if (n < 1 || !lua_istable(L, 1))
		return luaL_error(L, "Invalid argument, should be table");

	GLuint shaders[3];
	const char *str = NULL;
	int num = 0;

	if (lua_isstring(L, lua_getfield(L, 1, "VertexShader"))) {
		const char *str = lua_tostring(L, lua_gettop(L));
		shaders[num] = shader_from_strs(GL_VERTEX_SHADER, &str, 1);
		if (!shaders[num])
			luaL_error(L, "Could not compile vertex shader");
		num++;
	}

	if (lua_isstring(L, lua_getfield(L, 1, "GeometryShader"))) {
		const char *str = lua_tostring(L, lua_gettop(L));
		shaders[num] = shader_from_strs(GL_GEOMETRY_SHADER, &str, 1);
		if (!shaders[num])
			luaL_error(L, "Could not compile geometry shader");
		num++;
	}

	if (lua_isstring(L, lua_getfield(L, 1, "FragmentShader"))) {
		const char *str = lua_tostring(L, lua_gettop(L));
		shaders[num] = shader_from_strs(GL_FRAGMENT_SHADER, &str, 1);
		if (!shaders[num])
			luaL_error(L, "Could not compile vertex shader");
		num++;
	}

	GLuint shader_program = glsw_new_shader_program(shaders, num);
	if (!shader_program)
		luaL_error(L, "Could not link shader program");

	lua_pushnumber(L, shader_program);
	return 1;
}

lua_Number uniformHandle(const char *uniformName)