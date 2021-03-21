#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "lua_configuration.h"

void luaconf_run(lua_State *L, const char *filepath)
{
	if (filepath) {
		if (luaL_dofile(L, filepath)) {
			luaconf_error(L, "cannot run config. file: %s", lua_tostring(L, -1));
			lua_pop(L, 1);
		}
	} else {
		//TODO: Error handling
		luaL_dostring(L, "dofile(luaconf_path)");
	}
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