#ifndef LUA_CONFIGURATION_LUA_H
#define LUA_CONFIGURATION_LUA_H
#include <lua.h>
#include <stdbool.h>

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

//Generic get global, based on the type of the default value.
//The bool one doesn't seem to work, actually :(
#define getglob(L, var, d) _Generic((d), \
	bool:         getglobbool, \
	int:          getglobint, \
	lua_Integer:  getglobint, \
	lua_Number:   getglobnum, \
	char *: getglobstr)(L, var, d)


#endif