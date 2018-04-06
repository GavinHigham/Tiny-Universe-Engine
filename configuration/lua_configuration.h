#ifndef LUA_CONFIGURATION_LUA_H
#define LUA_CONFIGURATION_LUA_H
#include <lua.h>
#include <stdbool.h>

//Runs the Lua file located at filepath, and prints errors as appropriate.
void luaconf_run(lua_State *L, const char *filepath);
//Just prints an error message. Should rename this in the future when I'm feeling more creative.
void luaconf_error(lua_State *L, const char *fmt, ...);
//Returns the global bool with name var from L, or d if var could not be found.
bool getglobbool(lua_State *L, const char *var, bool d);
//Returns the global int with name var from L, or d if var could not be found.
int getglobint(lua_State *L, const char *var, lua_Integer d);
//Returns the global lua_Number with name var from L, or d if var could not be found.
lua_Number getglobnum(lua_State *L, const char *var, lua_Number d);
//Returns the global string with name var from L, or d if var could not be found.
char * getglobstr(lua_State *L, const char *var, const char *d);
//Fills buf (if not NULL) with the global string with name var from L, or d if var could not be found.
//Includes terminating NULL. Returns number of characters copied, including terminating NULL.
//Can be used with variable length arrays like so:
//	char tmp_str[gettmpglobstr(L, "var_name", "default_val", NULL)];
//	gettmpglobstr(L, "var_name", "default_val", tmp_str);
size_t gettmpglobstr(lua_State *L, const char *var, const char *d, char *buf);


//Generic get global, based on the type of the default value.
//The bool one doesn't seem to work, actually :(
#define getglob(L, var, d) _Generic((d), \
	bool:         getglobbool, \
	int:          getglobint, \
	lua_Integer:  getglobint, \
	lua_Number:   getglobnum, \
	char *: getglobstr)(L, var, d)


#endif