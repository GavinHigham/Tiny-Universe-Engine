#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "lua_configuration.h"

void luaconf_run(lua_State *L, const char *basepath, const char *filepath)
{
	if (!filepath)
		return;

	int top = lua_gettop(L);

	if (basepath) {
		lua_pushstring(L, basepath);
		lua_pushstring(L, filepath);
		lua_concat(L, 2);
		filepath = lua_tostring(L, -1);
	}

	if (luaL_dofile(L, filepath))
		luaconf_error(L, "cannot run config. file: %s", lua_tostring(L, -1));

	lua_settop(L, top);
}

//From the examples in "Programming in Lua: Fourth edition"

void luaconf_error(lua_State *L, const char *fmt, ...)
{
	va_list argp;
	va_start(argp, fmt);
	vfprintf(stderr, fmt, argp);
	va_end(argp);
	//lua_close(L); //Maybe we don't want to brutally abort everything at the slightest sign of trouble...
	//exit(EXIT_FAILURE);
}

int luaconf_register_builtin_lib(lua_State *L, lua_CFunction luaopen_fn, const char *name)
{
	int top = lua_gettop(L);
#define return_error(e, ...) do {luaconf_error(L, __VA_ARGS__); lua_settop(L, top); return e;} while (false)

	//Push a function on the stack, to which we can pass the searcher function (or nil) and builtin loaders table.
	//This function will create and insert the searcher function in package.searchers if it does not yet exist.
	if (luaL_dostring(L,
		"return function(searcher_fn, builtin_loaders)\n"
			"local searcher_fn = searcher_fn or function(name) return builtin_loaders[name] end\n"
			"for i,v in ipairs(package.searchers) do\n"
				"if v == searcher_fn then\n"
					"return searcher_fn\n"
				"end\n"
			"end\n"
			"--Did not find searcher function, add it to package.searchers\n"
			"table.insert(package.searchers, searcher_fn)\n"
			"return searcher_fn\n"
		"end\n")) {
		return_error(-1, "could not create the builtin loaders searcher factory function: %s\n", lua_tostring(L, -1));
	}

	if (!lua_isfunction(L, -1)) {
		return_error(-1, "could not create or retrieve builtin loaders searcher function\n");
	}

	//Get the builtin loaders searcher function (or nil)
	lua_getfield(L, LUA_REGISTRYINDEX, "tu_builtin_loaders_searcher_fn");

	//Get the builtin loaders table
	if (lua_getfield(L, LUA_REGISTRYINDEX, "tu_builtin_loaders") != LUA_TTABLE) {
		//The loaders table does not exist yet, create it
		lua_pop(L, 1); //First pop the nil we just stuck on the stack
		lua_newtable(L);
		lua_setfield(L, LUA_REGISTRYINDEX, "tu_builtin_loaders");
		if (lua_getfield(L, LUA_REGISTRYINDEX, "tu_builtin_loaders") != LUA_TTABLE) {
			return_error(-1, "could not create builtin loaders table\n");
		}
	}

	/*
	Stack should now be (relative to "top"):
	+1 | builtin loaders searcher factory function
	+2 | builtin loaders searcher function (or nil)
	+3 | builtin loaders table
	*/
	//Register the C loader
	lua_pushcfunction(L, luaopen_fn);
	lua_setfield(L, -2, name);

	//Call our factory to create or retrieve the searcher function
	if (lua_pcall(L, 2, 1, 0) != LUA_OK) {
		return_error(-1, "the builtin loaders searcher factory function failed: %s\n", lua_tostring(L, -1));
	}

	if (!lua_isfunction(L, -1)) {
		return_error(-1, "the builtin loaders searcher factory function succeeded, but did not return a valid searcher\n");
	}
	/*
	Stack should now be (relative to "top"):
	+1 | builtin loaders searcher function
	*/
	lua_setfield(L, LUA_REGISTRYINDEX, "tu_builtin_loaders_searcher_fn");
	//The stack should be empty, but reset it as a precaution.
	lua_settop(L, top);
	return 0;
}

void luaconf_unregister_builtin_lib(lua_State *L, const char *name)
{
	//Get the builtin loaders table
	if (lua_getfield(L, LUA_REGISTRYINDEX, "tu_builtin_loaders") == LUA_TTABLE) {
		lua_pushnil(L);
		lua_setfield(L, -2, name);
	}
}

bool getglobbool(lua_State *L, const char *var, bool d)
{
	lua_getglobal(L, var);
	bool result = lua_isboolean(L, -1) ? lua_toboolean(L, -1) : d;
	lua_pop(L, 1);
	return result;
}

int getglobint(lua_State *L, const char *var, lua_Integer d)
{
	lua_getglobal(L, var);
	int result = (int)luaL_optinteger(L, -1, d);
	lua_pop(L, 1);
	return result;
}

lua_Number getglobnum(lua_State *L, const char *var, lua_Number d)
{
	lua_getglobal(L, var);
	lua_Number result = luaL_optnumber(L, -1, d);
	lua_pop(L, 1);
	return result;
}

char * getglobstr(lua_State *L, const char *var, const char *d)
{
	lua_getglobal(L, var);
	char *result = (char *)luaL_optstring(L, -1, d);
	result = result ? strdup(result) : NULL;
	lua_pop(L, 1);
	return result;
}

size_t gettmpglobstr(lua_State *L, const char *var, const char *d, char *buf)
{
	lua_getglobal(L, var);
	char *result = (char *)luaL_optstring(L, -1, d);
	if (buf && result) {
		strcpy(buf, result);
		lua_pop(L, 1);
	} else if (result) {
		return strlen(result) + 1;
	}
	return 0;
}

bool getoptbool(lua_State *L, int i, bool d)
{
	bool result = lua_isboolean(L, i) ? lua_toboolean(L, i) : d;
	return result;
}

char * getoptstr(lua_State *L, int i, const char *d)
{
	char *result = (char *)luaL_optstring(L, i, d);
	result = result ? strdup(result) : NULL;
	return result;
}

bool getoptfieldbool(lua_State *L, int i, const char *var, bool d)
{
	lua_getfield(L, i, var);
	bool result = getopttop(L, d);
	lua_pop(L, 1);
	return result;
}
int getoptfieldint(lua_State *L, int i, const char *var, lua_Integer d)
{
	lua_getfield(L, i, var);
	int result = getopttop(L, d);
	lua_pop(L, 1);
	return result;
}
lua_Number getoptfieldnum(lua_State *L, int i, const char *var, lua_Number d)
{
	lua_getfield(L, i, var);
	lua_Number result = getopttop(L, d);
	lua_pop(L, 1);
	return result;
}
char * getoptfieldstr(lua_State *L, int i, const char *var, const char *d)
{
	lua_getfield(L, i, var);
	char *result = getopttop(L, d);
	lua_pop(L, 1);
	return result;
}