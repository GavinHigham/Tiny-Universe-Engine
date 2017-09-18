#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <lua.h>
#include <lauxlib.h>
#include "lua_configuration.h"

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
