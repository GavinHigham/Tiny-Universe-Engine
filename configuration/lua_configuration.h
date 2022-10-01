#ifndef LUA_CONFIGURATION_LUA_H
#define LUA_CONFIGURATION_LUA_H
#include <lua-5.4.4/src/lua.h>
#include <lua-5.4.4/src/lauxlib.h>
#include <stdbool.h>

//Runs the Lua file located at filepath, and prints errors as appropriate.
//If basepath is provided, it is used as a prefix to the filepath. 
void luaconf_run(lua_State *L, const char *basepath, const char *filepath);
//Just prints an error message. Should rename this in the future when I'm feeling more creative.
void luaconf_error(lua_State *L, const char *fmt, ...);
//Returns the global bool with name var from L, or d if var could not be found.
bool getglobbool(lua_State *L, const char *var, bool d);
//Returns the global int with name var from L, or d if var could not be found.
int getglobint(lua_State *L, const char *var, lua_Integer d);
//Returns the global lua_Number with name var from L, or d if var could not be found.
lua_Number getglobnum(lua_State *L, const char *var, lua_Number d);
//Returns the global string with name var from L, or d if var could not be found.
//Return value should be freed with free().
char * getglobstr(lua_State *L, const char *var, const char *d);
//Fills buf (if not NULL) with the global string with name var from L, or d if var could not be found.
//Includes terminating NULL. Returns number of characters copied, including terminating NULL.
//Can be used with variable length arrays like so:
//	char tmp_str[gettmpglobstr(L, "var_name", "default_val", NULL)];
//	gettmpglobstr(L, "var_name", "default_val", tmp_str);
size_t gettmpglobstr(lua_State *L, const char *var, const char *d, char *buf);

//Returns the bool at index i on L's stack, or d if that value is not a bool.
bool getoptbool(lua_State *L, int i, bool d);
//Returns the string at index i on L's stack, or d if that value is not a string.
//The result should be freed with free().
char * getoptstr(lua_State *L, int i, const char *d);

//Returns the field bool with name var from L, or d if var could not be found.
bool getoptfieldbool(lua_State *L, int i, const char *var, bool d);
//Returns the field int with name var from L, or d if var could not be found.
int getoptfieldint(lua_State *L, int i, const char *var, lua_Integer d);
//Returns the field lua_Number with name var from L, or d if var could not be found.
lua_Number getoptfieldnum(lua_State *L, int i, const char *var, lua_Number d);
//Returns the field string with name var from L, or d if var could not be found.
//Return value should be freed with free().
char * getoptfieldstr(lua_State *L, int i, const char *var, const char *d);

//Generic get field value, based on the type of the default value.
#define getoptfield(L, index, var, d) _Generic((d), \
	bool:         getoptfieldbool, \
	int:          getoptfieldint, \
	lua_Integer:  getoptfieldint, \
	lua_Number:   getoptfieldnum, \
	float:        getoptfieldnum, \
	char *:       getoptfieldstr)(L, index, var, d)

#define getopttopfield(L, var, d) getoptfield(L, -1, var, d)

//Generic get top value, based on the type of the default value.
#define getopttop(L, d) _Generic((d), \
	bool:         getoptbool, \
	int:          (int)luaL_optinteger, \
	lua_Integer:  luaL_optinteger, \
	lua_Number:   luaL_optnumber, \
	float:        luaL_optnumber, \
	const char *: getoptstr, \
	char *:       getoptstr)(L, -1, d)

//Generic get global, based on the type of the default value.
//The bool one doesn't seem to work, actually :(
#define getglob(L, var, d) _Generic((d), \
	bool:         getglobbool, \
	int:          getglobint, \
	lua_Integer:  getglobint, \
	lua_Number:   getglobnum, \
	float:        getglobnum, \
	char *:       getglobstr)(L, var, d)


#endif