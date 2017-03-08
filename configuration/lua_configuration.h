#ifndef LUA_CONFIGURATION_LUA_H
#define LUA_CONFIGURATION_LUA_H
#include <lua.h>
#include <stdbool.h>

void load_lua_config(lua_State *L, const char *fname, lua_Number *w, lua_Number *h, char **title, bool *fullscreen);


#endif