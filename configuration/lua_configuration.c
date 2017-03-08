#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <lua.h>
#include <lauxlib.h>
#include "lua_configuration.h"

//From the examples in "Programming in Lua: Fourth edition"

void engine_lua_error(lua_State *L, const char *fmt, ...)
{
	va_list argp;
	va_start(argp, fmt);
	vfprintf(stderr, fmt, argp);
	va_end(argp);
	//lua_close(L); //Maybe we don't want to brutally abort everything at the slightest sign of trouble...
	//exit(EXIT_FAILURE);
}

bool getglobbool(lua_State *L, const char *var)
{
	int result;
	lua_getglobal(L, var);
	result = lua_toboolean(L, -1);
	lua_pop(L, 1);
	return result;
}

//Get a global int from the Lua runtime. Returns 0 on success, nonzero on failure.
int getglobint(lua_State *L, const char *var, int *out)
{
	int isnum, result;
	lua_getglobal(L, var);
	result = (int)lua_tointegerx(L, -1, &isnum);
	if (!isnum)
		engine_lua_error(L, "'%s' should be a number\n", var);
	else
		*out = result;
	lua_pop(L, 1);
	return !isnum;
}

//Get a global int from the Lua runtime. Returns 0 on success, nonzero on failure.
int getglobnum(lua_State *L, const char *var, lua_Number *out)
{
	int isnum;
	lua_Number result;
	lua_getglobal(L, var);
	result = lua_tonumberx(L, -1, &isnum);
	if (!isnum)
		engine_lua_error(L, "'%s' should be a number\n", var);
	else
		*out = result;
	lua_pop(L, 1);
	return !isnum;
}

//Get a global string from the Lua runtime. The returned string should be freed with free(). Returns NULL on failure.
char * getglobstr(lua_State *L, const char *var)
{
	char *tmp = NULL;
	lua_getglobal(L, var);
	const char *s = lua_tostring(L, -1);
	if (!s)
		engine_lua_error(L, "'%s' should be a string\n", var);
	else
		tmp = strdup(s);
	lua_pop(L, 1);
	return tmp;
}

//Example config load function. I will probably want to replace this with something more general in the future.
void load_lua_config(lua_State *L, const char *fname, lua_Number *w, lua_Number *h, char **title, bool *fullscreen)
{
	if (luaL_loadfile(L, fname) || lua_pcall(L, 0, 0, 0)) {
		engine_lua_error(L, "cannot run config. file: %s", lua_tostring(L, -1));
		lua_pop(L, 1);
	}
	getglobnum(L, "screen_width", w);
	getglobnum(L, "screen_height", h);
	char *tmp = getglobstr(L, "screen_title");
	*title = tmp ? tmp : *title;
	*fullscreen = getglobbool(L, "fullscreen");
}