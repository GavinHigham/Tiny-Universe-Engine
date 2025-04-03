//Lua headers
#include <lua-5.4.4/src/lua.h>
#include <lua-5.4.4/src/lauxlib.h>
#include <lua-5.4.4/src/lualib.h>

#include "input_event.h"

static int l_isKeyDown(lua_State *L)
{
	lua_pushboolean(L, key_down(SDL_GetScancodeFromKey(SDL_GetKeyFromName((luaL_checkstring(L, 1))), NULL)));
    return 1;
}

static int l_isKeyPressed(lua_State *L)
{
	lua_pushboolean(L, key_pressed(SDL_GetScancodeFromKey(SDL_GetKeyFromName((luaL_checkstring(L, 1))), NULL)));
    return 1;
}

static int l_isScancodeDown(lua_State *L)
{
	lua_pushboolean(L, key_down(SDL_GetScancodeFromName(luaL_checkstring(L, 1))));
    return 1;
}

static int l_isModifierDown(lua_State *L)
{
	const char *modstrs[] = {
		"none",
		"lshift",
		"rshift",
		"lctrl",
		"rctrl",
		"lalt",
		"ralt",
		"lgui",
		"rgui",
		"num",
		"caps",
		"mode",
		"scroll",
		"ctrl",
		"shift",
		"alt",
		"gui",
	};

	const uint32_t mods[] = {
		0x0000,
		0x0001,
		0x0002,
		0x0040,
		0x0080,
		0x0100,
		0x0200,
		0x0400,
		0x0800,
		0x1000,
		0x2000,
		0x4000,
		0x8000,
		SDL_KMOD_LCTRL | SDL_KMOD_RCTRL,
		SDL_KMOD_LSHIFT | SDL_KMOD_RSHIFT,
		SDL_KMOD_LALT | SDL_KMOD_RALT,
		SDL_KMOD_LGUI | SDL_KMOD_RGUI,
	};

	//Map option string to modifier flag, check against modifier state.
	//Ex. 'lshift' -> 1 -> 0x0001
	lua_pushboolean(L, mods[luaL_checkoption(L, 1, NULL, modstrs)] & SDL_GetModState());
	return 1;
}

static int l_isScancodePressed(lua_State *L)
{
	lua_pushboolean(L, key_pressed(SDL_GetScancodeFromName(luaL_checkstring(L, 1))));
    return 1;
}

static int l_scancodeDirectional(lua_State *L)
{
	int top = lua_gettop(L);
	for (int i = 1; i < top; i+=2) {
		lua_pushnumber(L, key_down(SDL_GetScancodeFromName(luaL_checkstring(L, i))) - key_down(SDL_GetScancodeFromName(luaL_checkstring(L, i+1))));
	}
	return top / 2;
}

static int l_mouseForUI(lua_State *L)
{
	float mouse_x, mouse_y;
	uint32_t buttons_held = SDL_GetMouseState(&mouse_x, &mouse_y);
	bool button = buttons_held & SDL_BUTTON_MASK(SDL_BUTTON_LEFT);
	int scroll_x = input_mouse_wheel_sum.wheel.x, scroll_y = input_mouse_wheel_sum.wheel.y;
	lua_pushinteger(L, mouse_x);
	lua_pushinteger(L, mouse_y);
	lua_pushboolean(L, button);
	lua_pushinteger(L, scroll_x);
	lua_pushinteger(L, scroll_y);
	return 5;
}

int luaopen_l_sdl_input(lua_State *L)
{
	luaL_Reg l_sdl_input[] = {
		{"isKeyDown", l_isKeyDown},
		{"isKeyPressed", l_isKeyPressed},
		{"isModifierDown", l_isModifierDown},
		{"isScancodeDown", l_isScancodeDown},
		{"isScancodePressed", l_isScancodePressed},
		{"scancodeDirectional", l_scancodeDirectional},
		{"mouseForUI", l_mouseForUI},
		{NULL, NULL}
	};

	luaL_newlib(L, l_sdl_input);
	return 1;
}