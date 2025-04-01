#include <glla.h>
#include <lua-5.4.4/src/lua.h>
#include <lua-5.4.4/src/lauxlib.h>
#include <lua-5.4.4/src/lualib.h>
#include "math/utility.h"

/*
What kind of syntax should I go for with this?

Ideas:

local v1 = glla.vec3() --Explicit size constructor
local v2 = glla.vec(1,2,3) --Variable size constructor

local v3 = glla.normalize(v2) --Procedural-style functions
v2:normalize() --Metatable-based methods. Nice because I can avoid prepending the lib table
v2.normalize() --closure-based methods. Possibly more familiar, possibly faster,
-- more memory use, stranger for Lua-natives.

local v4 = v2.normalize() --Does it mutate v2 or just return a mutation?
--Maybe return mutation for '.', modify original with ':'

--Going to need operator overloading too:
local v5 = v2.normalize() + v1

--How will I handle swizzling? I guess it will have to manually
--access components by index 

--[[
If every vector has a whole table of every function, they will end up being very expensive
that necessitates : based methods.

]]
*/

static int l_vec2_new(lua_State *L)
{
	lua_Number x = luaL_checknumber(L, 1);
	lua_Number y = luaL_checknumber(L, 2);
	vec2 *v = lua_newuserdatauv(L, sizeof(vec2), 0);
	*v = (vec2){x, y};
	luaL_setmetatable(L, "tu.vec2");

	return 1;
}

static int l_vec3_new(lua_State *L)
{
	//Since this is called with __call, arg 1 is the callable object
	lua_Number x = luaL_checknumber(L, 2);
	lua_Number y = luaL_checknumber(L, 3);
	lua_Number z = luaL_checknumber(L, 4);
	vec3 *v = lua_newuserdatauv(L, sizeof(vec3), 0);
	*v = (vec3){x, y, z};
	luaL_setmetatable(L, "tu.vec3");

	return 1;
}
static int l_vec3__index(lua_State *L)
{
	vec3 *v = luaL_checkudata(L, 1, "tu.vec3");
	vec3 *out = lua_newuserdatauv(L, sizeof(vec3), 0);

	size_t len = 0;
	const char *idx = luaL_checklstring(L, 2, &len);

	for (int i = 0; i < len && i < 3; i++) { //Is it worth unrolling this loop?
		switch (idx[i]) {
		case 'x': //fall-through
		case '0': (*out)[i] = v->x; break;
		case 'y': //fall-through
		case '1': (*out)[i] = v->y; break;
		case 'z': //fall-through
		case '2': (*out)[i] = v->z; break;
		//Trying to access pow, mod, etc.
		default: goto metafield;
		}
	}

	//TODO: Implement vec2, call the constructor and return one.
	switch(len) {
	case 0:
		luaL_error(L, "Unsupported swizzle mask length");
		return 0;
	case 1:
		lua_pushnumber(L, out->x);
		return 1;
	case 3:
		luaL_setmetatable(L, "tu.vec3");
		return 1;
	default:
	metafield:
		luaL_getmetafield(L, 1, idx); return 1;
	}
}
static int l_vec3__newindex(lua_State *L)
{
	vec3 *v = luaL_checkudata(L, 1, "tu.vec3");
	vec3 *v2 = NULL;
	size_t len = 0;
	const char *idx = luaL_checklstring(L, 2, &len);
	switch(len) {	
	case 1:
		switch (idx[0]) {
		case 'x': //fall-through
		case '0': v->x = luaL_checknumber(L, 3); break;
		case 'y': //fall-through
		case '1': v->y = luaL_checknumber(L, 3); break;
		case 'z': //fall-through
		case '2': v->z = luaL_checknumber(L, 3); break;
		default: luaL_error(L, "Invalid index"); break;
		} break;
	case 3:
		v2 = luaL_checkudata(L, 3, "tu.vec3");
		for (int i = 0; i < 3; i++) {
			switch (idx[i]) {
			case 'x': //fall-through
			case '0': v->x = (*v2)[i]; break;
			case 'y': //fall-through
			case '1': v->y = (*v2)[i]; break;
			case 'z': //fall-through
			case '2': v->z = (*v2)[i]; break;
			default: luaL_error(L, "Invalid index"); break;
			}
		} break;
	case 0: //fall-through
	//TODO: Implement vec2, call the constructor and return one.
	case 2:
	default:
		luaL_error(L, "Unsupported swizzle mask length (%d)", len); break;
	}
	return 0;
}
static int l_vec3__tostring(lua_State *L)
{
	// Get the vec3 object from the arguments
	vec3 *v = luaL_checkudata(L, 1, "tu.vec3");
	luaL_Buffer b;
	luaL_buffinit(L, &b);

	luaL_addstring(&b, "vec3(");
	lua_pushnumber(L, v->x);
	luaL_addvalue(&b);
	luaL_addstring(&b, ", ");
	lua_pushnumber(L, v->y);
	luaL_addvalue(&b);
	luaL_addstring(&b, ", ");
	lua_pushnumber(L, v->z);
	luaL_addvalue(&b);
	luaL_addstring(&b, ")");

	luaL_pushresult(&b);
	return 1;
}
//TODO: Allow scalar operands
static int l_vec3_add(lua_State *L)
{
	vec3 *a = luaL_checkudata(L, 1, "tu.vec3");
	vec3 *b = luaL_checkudata(L, 2, "tu.vec3");
	vec3 *c = lua_newuserdatauv(L, sizeof(vec3), 0);
	*c = *a + *b;
	luaL_setmetatable(L, "tu.vec3");
	return 1;
}
static int l_vec3_sub(lua_State *L)
{
	vec3 *a = luaL_checkudata(L, 1, "tu.vec3");
	vec3 *b = luaL_checkudata(L, 2, "tu.vec3");
	vec3 *c = lua_newuserdatauv(L, sizeof(vec3), 0);
	*c = *a - *b;
	luaL_setmetatable(L, "tu.vec3");
	return 1;
}
static int l_vec3_mul(lua_State *L)
{
	vec3 *a = luaL_checkudata(L, 1, "tu.vec3");
	mat3 *m = luaL_testudata(L, 2, "tu.mat3");
	if (m) { //Second argument is a mat3, multiply vec from right.
		vec3 *c = lua_newuserdatauv(L, sizeof(vec3), 0);
		*c = vec3_multmat3(*a, *m);
		luaL_setmetatable(L, "tu.vec3");
		return 1;
	}
	//Second argument is a vec3, component-wise multiplication.
	vec3 *b = luaL_testudata(L, 2, "tu.vec3");
	if (b) {
		vec3 *c = lua_newuserdatauv(L, sizeof(vec3), 0);
		*c = *a * *b;
		luaL_setmetatable(L, "tu.vec3");
		return 1;
	}
	//Secord argument is a number
	lua_Number n = luaL_checknumber(L, 2);
	vec3 *c = lua_newuserdatauv(L, sizeof(vec3), 0);
	*c = *a * n;
	luaL_setmetatable(L, "tu.vec3");
	return 1;
}
static int l_vec3_div(lua_State *L)
{
	vec3 *a = luaL_checkudata(L, 1, "tu.vec3");
	vec3 *b = luaL_checkudata(L, 2, "tu.vec3");
	vec3 *c = lua_newuserdatauv(L, sizeof(vec3), 0);
	*c = *a / *b;
	luaL_setmetatable(L, "tu.vec3");
	return 1;
}
//TODO: __mod, __pow, __unm, (__idiv, __band, __bor, __bxor, __bnot, __shl, __shr)?


//Returns a new vector pointing in the same direction which has been normalized (magnitude set to 1.0)
static int l_vec3_normalize(lua_State *L)
{
	vec3 *a = luaL_checkudata(L, 1, "tu.vec3");
	*a = vec3_normalize(*a);
	return 1;
}
//Returns a new vector pointing in the same direction which has been normalized (magnitude set to 1.0)
static int l_vec3_normalized(lua_State *L)
{
	vec3 *a = luaL_checkudata(L, 1, "tu.vec3");
	vec3 *c = lua_newuserdatauv(L, sizeof(vec3), 0);
	*c = vec3_normalize(*a);
	luaL_setmetatable(L, "tu.vec3");
	return 1;
}
//Returns a new vector that represents the cross product of a and b.
static int l_vec3_cross(lua_State *L)
{
	vec3 *a = luaL_checkudata(L, 1, "tu.vec3");
	vec3 *b = luaL_checkudata(L, 2, "tu.vec3");
	vec3 *c = lua_newuserdatauv(L, sizeof(vec3), 0);
	*c = vec3_cross(*a, *b);
	luaL_setmetatable(L, "tu.vec3");
	return 1;
}
//Returns a new vector that is linearly interpolated between a and by by parameter alpha.
//(a*alpha + b*(1 - alpha)
static int l_vec3_lerp(lua_State *L)
{
	vec3 *a = luaL_checkudata(L, 1, "tu.vec3");
	vec3 *b = luaL_checkudata(L, 2, "tu.vec3");
	lua_Number alpha = luaL_checknumber(L, 3);
	vec3 *c = lua_newuserdatauv(L, sizeof(vec3), 0);
	*c = vec3_lerp(*a, *b, alpha);
	luaL_setmetatable(L, "tu.vec3");
	return 1;
}
//Return the dot product of a and b.
static int l_vec3_dot(lua_State *L)
{
	vec3 *a = luaL_checkudata(L, 1, "tu.vec3");
	vec3 *b = luaL_checkudata(L, 1, "tu.vec3");
	lua_pushnumber(L, vec3_dot(*a, *b));
	return 1;
}
//Return the magnitude of a.
static int l_vec3_mag(lua_State *L)
{
	vec3 *a = luaL_checkudata(L, 1, "tu.vec3");
	lua_pushnumber(L, vec3_mag(*a));
	return 1;
}
//Return the distance between a and b.
static int l_vec3_dist(lua_State *L)
{
	vec3 *a = luaL_checkudata(L, 1, "tu.vec3");
	vec3 *b = luaL_checkudata(L, 2, "tu.vec3");
	lua_pushnumber(L, vec3_dist(*a, *b));
	return 1;
}

static int l_vec4_new(lua_State *L)
{
	//Since this is called with __call, arg 1 is the callable object
	lua_Number x = luaL_checknumber(L, 2);
	lua_Number y = luaL_checknumber(L, 3);
	lua_Number z = luaL_checknumber(L, 4);
	lua_Number w = luaL_checknumber(L, 5);
	vec4 *v = lua_newuserdatauv(L, sizeof(vec4), 0);
	*v = (vec4){x, y, z, w};
	luaL_setmetatable(L, "tu.vec4");

	return 1;
}
static int l_vec4__index(lua_State *L)
{
	vec4 *v = luaL_checkudata(L, 1, "tu.vec4");
	vec4 *out = lua_newuserdatauv(L, sizeof(vec4), 0);

	size_t len = 0;
	const char *idx = luaL_checklstring(L, 2, &len);

	for (int i = 0; i < len && i < 4; i++) { //Is it worth unrolling this loop?
		switch (idx[i]) {
		case 'x': //fall-through
		case '0': (*out)[i] = v->x; break;
		case 'y': //fall-through
		case '1': (*out)[i] = v->y; break;
		case 'z': //fall-through
		case '2': (*out)[i] = v->z; break;
		case 'w': //fall-through
		case '3': (*out)[i] = v->w; break;
		//Trying to access pow, mod, etc.
		default: goto metafield;
		}
	}

	//TODO: Implement vec2, call the constructor and return one.
	switch(len) {
	case 0:
		luaL_error(L, "Unsupported swizzle mask length");
		return 0;
	case 1:
		lua_pushnumber(L, out->x);
		return 1;
	case 3:
		luaL_setmetatable(L, "tu.vec3");
		return 1;
	case 4:
		luaL_setmetatable(L, "tu.vec4");
		return 1;
	default:
	metafield:
		luaL_getmetafield(L, 1, idx); return 1;
	}
}
static int l_vec4__newindex(lua_State *L)
{
	vec4 *v = luaL_checkudata(L, 1, "tu.vec4");
	vec3 *v3 = NULL;
	vec4 *v4 = NULL;
	size_t len = 0;
	const char *idx = luaL_checklstring(L, 2, &len);
	switch(len) {	
	case 1:
		switch (idx[0]) {
		case 'x': //fall-through
		case '0': v->x = luaL_checknumber(L, 3); break;
		case 'y': //fall-through
		case '1': v->y = luaL_checknumber(L, 3); break;
		case 'z': //fall-through
		case '2': v->z = luaL_checknumber(L, 3); break;
		case 'w': //fall-through
		case '3': v->w = luaL_checknumber(L, 3); break;
		default: luaL_error(L, "Invalid index"); break;
		} break;
	case 3:
		v3 = luaL_checkudata(L, 3, "tu.vec3");
		for (int i = 0; i < 3; i++) {
			switch (idx[i]) {
			case 'x': //fall-through
			case '0': v->x = (*v3)[i]; break;
			case 'y': //fall-through
			case '1': v->y = (*v3)[i]; break;
			case 'z': //fall-through
			case '2': v->z = (*v3)[i]; break;
			case 'w': //fall-through
			case '3': v->w = (*v3)[i]; break;
			default: luaL_error(L, "Invalid index"); break;
			}
		} break;
	case 4:
		v4 = luaL_checkudata(L, 3, "tu.vec4");
		for (int i = 0; i < 4; i++) {
			switch (idx[i]) {
			case 'x': //fall-through
			case '0': v->x = (*v4)[i]; break;
			case 'y': //fall-through
			case '1': v->y = (*v4)[i]; break;
			case 'z': //fall-through
			case '2': v->z = (*v4)[i]; break;
			case 'w': //fall-through
			case '3': v->w = (*v4)[i]; break;
			default: luaL_error(L, "Invalid index"); break;
			}
		} break;
	case 0: //fall-through
	//TODO: Implement vec2, call the constructor and return one.
	case 2:
	default:
		luaL_error(L, "Unsupported swizzle mask length (%d)", len); break;
	}
	return 0;
}
static int l_vec4__tostring(lua_State *L)
{
	// Get the vec3 object from the arguments
	vec4 *v = luaL_checkudata(L, 1, "tu.vec4");
	luaL_Buffer b;
	luaL_buffinit(L, &b);

	luaL_addstring(&b, "vec4(");
	lua_pushnumber(L, v->x);
	luaL_addvalue(&b);
	luaL_addstring(&b, ", ");
	lua_pushnumber(L, v->y);
	luaL_addvalue(&b);
	luaL_addstring(&b, ", ");
	lua_pushnumber(L, v->z);
	luaL_addvalue(&b);
	luaL_addstring(&b, ", ");
	lua_pushnumber(L, v->w);
	luaL_addvalue(&b);
	luaL_addstring(&b, ")");

	luaL_pushresult(&b);
	return 1;
}
//TODO: Allow scalar operands
static int l_vec4_add(lua_State *L)
{
	vec4 *a = luaL_checkudata(L, 1, "tu.vec4");
	vec4 *b = luaL_checkudata(L, 2, "tu.vec4");
	vec4 *c = lua_newuserdatauv(L, sizeof(vec4), 0);
	*c = *a + *b;
	luaL_setmetatable(L, "tu.vec4");
	return 1;
}
static int l_vec4_sub(lua_State *L)
{
	vec4 *a = luaL_checkudata(L, 1, "tu.vec4");
	vec4 *b = luaL_checkudata(L, 2, "tu.vec4");
	vec4 *c = lua_newuserdatauv(L, sizeof(vec4), 0);
	*c = *a - *b;
	luaL_setmetatable(L, "tu.vec4");
	return 1;
}
static int l_vec4_mul(lua_State *L)
{
	vec4 *a = luaL_checkudata(L, 1, "tu.vec4");
	// amat4 *m = luaL_testudata(L, 2, "tu.amat4");
	// if (m) { //Second argument is an amat4, multiply vec from right.
	// 	vec3 *c = lua_newuserdatauv(L, sizeof(vec3), 0);
	// 	*c = vec3_multmat3(*a, *m);
	// 	luaL_setmetatable(L, "tu.vec3");
	// 	return 1;
	// }
	//Second argument is a vec3, component-wise multiplication.
	vec4 *b = luaL_checkudata(L, 2, "tu.vec4");
	vec4 *c = lua_newuserdatauv(L, sizeof(vec4), 0);
	*c = *a * *b;
	luaL_setmetatable(L, "tu.vec4");
	return 1;
}
static int l_vec4_div(lua_State *L)
{
	vec4 *a = luaL_checkudata(L, 1, "tu.vec4");
	vec4 *b = luaL_checkudata(L, 2, "tu.vec4");
	vec4 *c = lua_newuserdatauv(L, sizeof(vec4), 0);
	*c = *a / *b;
	luaL_setmetatable(L, "tu.vec4");
	return 1;
}
//TODO: __mod, __pow, __unm, (__idiv, __band, __bor, __bxor, __bnot, __shl, __shr)?

//TODO: Copy more of the vec3 functions and implement for vec4, as needed
//Returns a new vector pointing in the same direction which has been normalized (magnitude set to 1.0)
// static int l_vec4_normalize(lua_State *L)
// {
// 	vec4 *a = luaL_checkudata(L, 1, "tu.vec4");
// 	vec4 *c = lua_newuserdatauv(L, sizeof(vec4), 0);
// 	*c = vec4_normalize(*a);
// 	luaL_setmetatable(L, "tu.vec4");
// 	return 1;
// }
//Returns a new vector that represents the cross product of a and b.
// static int l_vec4_cross(lua_State *L)
// {
// 	vec4 *a = luaL_checkudata(L, 1, "tu.vec4");
// 	vec4 *b = luaL_checkudata(L, 2, "tu.vec4");
// 	vec4 *c = lua_newuserdatauv(L, sizeof(vec4), 0);
// 	*c = vec4_cross(*a, *b);
// 	luaL_setmetatable(L, "tu.vec4");
// 	return 1;
// }
//Returns a new vector that is linearly interpolated between a and by by parameter alpha.
//(a*alpha + b*(1 - alpha)
// static int l_vec4_lerp(lua_State *L)
// {
// 	vec4 *a = luaL_checkudata(L, 1, "tu.vec4");
// 	vec4 *b = luaL_checkudata(L, 2, "tu.vec4");
// 	lua_Number alpha = luaL_checknumber(L, 3);
// 	vec4 *c = lua_newuserdatauv(L, sizeof(vec4), 0);
// 	*c = vec4_lerp(*a, *b, alpha);
// 	luaL_setmetatable(L, "tu.vec4");
// 	return 1;
// }
//Return the dot product of a and b.
// static int l_vec4_dot(lua_State *L)
// {
// 	vec4 *a = luaL_checkudata(L, 1, "tu.vec4");
// 	vec4 *b = luaL_checkudata(L, 1, "tu.vec4");
// 	lua_pushnumber(L, vec4_dot(*a, *b));
// 	return 1;
// }
// //Return the magnitude of a.
// static int l_vec4_mag(lua_State *L)
// {
// 	vec4 *a = luaL_checkudata(L, 1, "tu.vec4");
// 	lua_pushnumber(L, vec4_mag(*a));
// 	return 1;
// }
// //Return the distance between a and b.
// static int l_vec4_dist(lua_State *L)
// {
// 	vec4 *a = luaL_checkudata(L, 1, "tu.vec4");
// 	vec4 *b = luaL_checkudata(L, 1, "tu.vec4");
// 	lua_pushnumber(L, vec4_dist(*a, *b));
// 	return 1;
// }

static int l_vec3_multmat3(lua_State *L)
{
	vec3 *a = luaL_checkudata(L, 1, "tu.vec3");
	mat3 *b = luaL_checkudata(L, 2, "tu.mat3");
	vec3 *c = lua_newuserdatauv(L, sizeof(vec3), 0);
	*c = vec3_multmat3(*a, *b);
	luaL_setmetatable(L, "tu.vec3");
	return 1;
}

static int l_mat3_new(lua_State *L)
{
	//Since this is called with __call, arg 1 is the callable object
	mat3 *a = lua_newuserdatauv(L, sizeof(mat3), 0);
	a->rows[0].x = luaL_checknumber(L, 2);
	a->rows[0].y = luaL_checknumber(L, 3);
	a->rows[0].z = luaL_checknumber(L, 4);
	a->rows[1].x = luaL_checknumber(L, 5);
	a->rows[1].y = luaL_checknumber(L, 6);
	a->rows[1].z = luaL_checknumber(L, 7);
	a->rows[2].x = luaL_checknumber(L, 8);
	a->rows[2].y = luaL_checknumber(L, 9);
	a->rows[2].z = luaL_checknumber(L, 10);
	luaL_setmetatable(L, "tu.mat3");
	return 1;
}

static int l_mat3_identity(lua_State *L)
{
	mat3 *a = lua_newuserdatauv(L, sizeof(mat3), 0);
	luaL_setmetatable(L, "tu.mat3");
	*a = (mat3)MAT3_IDENT;
	return 1;
}

static int l_mat3__index(lua_State *L)
{
	mat3 *a = luaL_checkudata(L, 1, "tu.mat3");
	size_t len = 0;
	const char *idx = luaL_checklstring(L, 2, &len);
	vec3 *out = lua_newuserdatauv(L, sizeof(vec3), 0);

	for (int i = 0; i < len; i++) { //Is it worth unrolling this loop?
		switch (idx[i]) {
		case '0': *out = a->rows[0]; break;
		case '1': *out = a->rows[1]; break;
		case '2': *out = a->rows[2]; break;
		//Trying to access pow, mod, etc.
		default: goto metafield;
		}
	}

	//TODO(Gavin): Support multidimensional indexing by parsing the index string
	//ex. funmatrix['0,1'] or funmatrix[[[23]]]
	switch(len) {
	case 0: //fall-through
		luaL_error(L, "Unsupported swizzle mask length"); return 0;
	case 1:
		luaL_setmetatable(L, "tu.vec3");
		return 1;
	default:
	metafield:
		luaL_getmetafield(L, 1, idx); return 1;
	}
}

static int l_mat3__newindex(lua_State *L)
{
	mat3 *a = luaL_checkudata(L, 1, "tu.mat3");
	size_t len = 0;
	const char *idx = luaL_checklstring(L, 2, &len);
	vec3 *out = lua_newuserdatauv(L, sizeof(vec3), 0);

	for (int i = 0; i < len; i++) { //Is it worth unrolling this loop?
		switch (idx[i]) {
		case '0': *out = a->rows[0]; break;
		case '1': *out = a->rows[1]; break;
		case '2': *out = a->rows[2]; break;
		//Trying to access pow, mod, etc.
		default: goto metafield;
		}
	}

	//TODO(Gavin): Support multidimensional indexing by parsing the index string
	//ex. funmatrix['0,1'] or funmatrix[[[23]]]
	switch(len) {
	case 0: //fall-through
		luaL_error(L, "Unsupported swizzle mask length"); return 0;
	case 1:
		luaL_setmetatable(L, "tu.vec3");
		return 1;
	default:
	metafield:
		luaL_getmetafield(L, 1, idx); return 1;
	}
}

static int l_mat3__mul(lua_State *L)
{
	mat3 *a = luaL_checkudata(L, 1, "tu.mat3");
	vec3 *v = luaL_testudata(L, 2, "tu.vec3");
	if (v) {
		vec3 *c = lua_newuserdatauv(L, sizeof(vec3), 0);
		*c = mat3_multvec(*a, *v);
		luaL_setmetatable(L, "tu.vec3");
		return 1;
	}
	mat3 *b = luaL_checkudata(L, 2, "tu.mat3");
	mat3 *c = lua_newuserdatauv(L, sizeof(mat3), 0);
	*c = mat3_mult(*a, *b);
	luaL_setmetatable(L, "tu.mat3");
	return 1;
}

static int l_mat3_multvec(lua_State *L)
{
	mat3 *a = luaL_checkudata(L, 1, "tu.mat3");
	vec3 *b = luaL_checkudata(L, 2, "tu.vec3");
	vec3 *c = lua_newuserdatauv(L, sizeof(vec3), 0);
	*c = mat3_multvec(*a, *b);
	luaL_setmetatable(L, "tu.vec3");
	return 1;
}

static int l_mat3_rot(lua_State *L)
{
	mat3 *a = luaL_checkudata(L, 1, "tu.mat3");
	float ux = luaL_checknumber(L, 2);
	float uy = luaL_checknumber(L, 3);
	float uz = luaL_checknumber(L, 4);
	float s = luaL_checknumber(L, 5);
	float c = luaL_checknumber(L, 6);
	mat3 *tmp = lua_newuserdatauv(L, sizeof(mat3), 0);
	*tmp = mat3_rot(*a, ux, uy, uz, s, c);
	luaL_setmetatable(L, "tu.mat3");
	return 1;
}

static int l_mat3_scale(lua_State *L)
{
	mat3 *a = luaL_checkudata(L, 1, "tu.mat3");
	float x = luaL_checknumber(L, 2);
	float y = luaL_checknumber(L, 3);
	float z = luaL_checknumber(L, 4);
	mat3 *c = lua_newuserdatauv(L, sizeof(mat3), 0);
	*c = mat3_scale(*a, x, y, z);
	luaL_setmetatable(L, "tu.mat3");
	return 1;
}

static int l_mat3_scalemat(lua_State *L)
{
	float x = luaL_checknumber(L, 1);
	float y = luaL_checknumber(L, 2);
	float z = luaL_checknumber(L, 3);
	mat3 *c = lua_newuserdatauv(L, sizeof(mat3), 0);
	*c = mat3_scalemat(x, y, z);
	luaL_setmetatable(L, "tu.mat3");
	return 1;
}

static int l_mat3_transpose(lua_State *L)
{
	mat3 *a = luaL_checkudata(L, 1, "tu.mat3");
	*a = mat3_transp(*a);
	return 0;
}

static int l_mat3_transposed(lua_State *L)
{
	mat3 *a = luaL_checkudata(L, 1, "tu.mat3");
	mat3 *c = lua_newuserdatauv(L, sizeof(mat3), 0);
	*c = mat3_transp(*a);
	luaL_setmetatable(L, "tu.mat3");
	return 1;
}

static int l_mat3_lookat(lua_State *L)
{
	vec3 *p = luaL_checkudata(L, 1, "tu.vec3");
	vec3 *q = luaL_checkudata(L, 2, "tu.vec3");
	vec3 *u = luaL_checkudata(L, 3, "tu.vec3");
	mat3 *c = lua_newuserdatauv(L, sizeof(mat3), 0);
	*c = mat3_lookat(*p, *q, *u);
	luaL_setmetatable(L, "tu.mat3");
	return 1;
}

// //Pushes a onto the Lua stack as a "tu.amat4" userdata value
// static void l_amat4_push(lua_State *L, amat4 a)
// {
// 	amat4 *m = lua_newuserdatauv(L, sizeof(amat4), 0);
// 	luaL_setmetatable(L, "tu.amat4");
// 	*m = a;
// }

static int l_amat4_new(lua_State *L)
{
	//Since this is called with __call, arg 1 is the callable object
	amat4 *m = lua_newuserdatauv(L, sizeof(amat4), 0);
	m->a = *(mat3 *)luaL_checkudata(L, 2, "tu.mat3");
	m->t = *(vec3 *)luaL_checkudata(L, 3, "tu.vec3");
	luaL_setmetatable(L, "tu.amat4");
	return 1;
}

static int l_amat4_identity(lua_State *L)
{
	amat4 *a = lua_newuserdatauv(L, sizeof(amat4), 0);
	luaL_setmetatable(L, "tu.amat4");
	*a = (amat4)AMAT4_IDENT;
	return 1;
}

static int l_amat4__index(lua_State *L)
{
	amat4 *a = luaL_checkudata(L, 1, "tu.amat4");
	size_t len = 0;
	const char *idx = luaL_checklstring(L, 2, &len);

	mat3 *a_a;
	vec3 *a_t;

	if (len == 0) {
		luaL_error(L, "Index with length 0 of amat4 does not make sense");
		return 0;
	}

	if (len == 1) switch (idx[0]) {
	case 'a':
		a_a = lua_newuserdatauv(L, sizeof(mat3), 0);
		luaL_setmetatable(L, "tu.mat3");
		*a_a = a->a;
		return 1;
	case 't':
		a_t = lua_newuserdatauv(L, sizeof(vec3), 0);
		luaL_setmetatable(L, "tu.vec3");
		*a_t = a->t;
		return 1;
	}

	int isnum = 0;
	lua_Integer n = lua_tonumberx(L, 2, &isnum);
	if (isnum) {
		if (n < 0 || n > 15) {
			luaL_error(L, "Index must be 0-15 (inclusive)");
			return 0;
		}
		lua_pushnumber(L, amat4_index(*a, n));
		return 1;
	}

	//Trying to access pow, mod, etc.
	luaL_getmetafield(L, 1, idx);
	return 1;
}

static int l_amat4__newindex(lua_State *L)
{
	amat4 *a = luaL_checkudata(L, 1, "tu.amat4");
	size_t len = 0;
	const char *idx = luaL_checklstring(L, 2, &len);

	if (len == 0) {
		luaL_error(L, "Index with length 0 of amat4 does not make sense");
		return 0;
	}

	if (len ==1) switch (idx[0]) {
	case 'a': a->a = *(mat3 *)luaL_checkudata(L, 3, "tu.mat3"); return 0;
	case 't': a->t = *(vec3 *)luaL_checkudata(L, 3, "tu.vec3"); return 0;
	}

	//TODO: Numeric indices
	return 0;
}

static int l_amat4_mult(lua_State *L)
{
	void *a = lua_touserdata(L, 1);
	void *b = lua_touserdata(L, 2);

	if (a && b) {
		lua_getmetatable(L, 1);
		lua_getmetatable(L, 2);
		luaL_getmetatable(L, "tu.amat4");

		if (lua_rawequal(L, -1, -3)) { //a is amat4
			if (lua_rawequal(L, -1, -2)) { //b is also amat4
				amat4 *c = lua_newuserdatauv(L, sizeof(amat4), 0);
				*c = amat4_mult(*(amat4 *)a, *(amat4 *)b);
				luaL_setmetatable(L, "tu.amat4");
				return 1;
			}
			luaL_getmetatable(L, "tu.mat4");
			if (lua_rawequal(L, -1, -3)) { //b is mat4
				float *out = lua_newuserdatauv(L, sizeof(float) * 16, 0);
				amat4_mat_buf_mult(*(amat4 *)a, b, out);
				luaL_setmetatable(L, "tu.mat4");
				return 1;
			}
			//b is unknown userdata
			luaL_argerror(L, 2, "Expected amat4 or mat4");
			return 0;
		}
		luaL_getmetatable(L, "tu.mat4");
		if (lua_rawequal(L, -1, -4)) { //a is mat4
			if (lua_rawequal(L, -2, -3)) { //b is amat4
				float *out = lua_newuserdatauv(L, sizeof(float) * 16, 0);
				amat4_buf_mat_mult(a, *(amat4 *)b, out);
				luaL_setmetatable(L, "tu.mat4");
				return 1;
			}
			if (lua_rawequal(L, -1, -3)) { //b is also mat4
				if (a == b) { //amat4_buf_mult uses restrict, undefined behavior if pointers overlap
					luaL_argerror(L, 2, "Cannot multiply a single instance of mat4 with itself");
					return 0;
				}
				float *out = lua_newuserdatauv(L, sizeof(float) * 16, 0);
				amat4_buf_mult(a, b, out);
				luaL_setmetatable(L, "tu.mat4");
				return 1;
			}
			//b is unknown userdata
			luaL_argerror(L, 2, "Expected amat4 or mat4");
			return 0;
		}
	}
	//a is unknown type
	luaL_argerror(L, 1, "Expected amat4 or mat4");
	return 0;
}

static int l_amat4_multpoint(lua_State *L)
{
	amat4 *a = luaL_checkudata(L, 1, "tu.amat4");
	vec3 *b = luaL_checkudata(L, 2, "tu.vec3");
	vec3 *c = lua_newuserdatauv(L, sizeof(vec3), 0);
	*c = amat4_multpoint(*a, *b);
	luaL_setmetatable(L, "tu.vec3");
	return 1;
}

static int l_mat4_multpoint(lua_State *L)
{
	float *a = luaL_checkudata(L, 1, "tu.mat4");
	vec3 *b = luaL_checkudata(L, 2, "tu.vec3");
	vec3 *c = lua_newuserdatauv(L, sizeof(vec3), 0);
	*c = amat4_buf_multpoint(a, (float [4]){b->x, b->y, b->z, 1.0}, NULL);
	luaL_setmetatable(L, "tu.vec3");
	return 1;
}

static int l_amat4_multvec(lua_State *L)
{
	amat4 *a = luaL_checkudata(L, 1, "tu.amat4");
	vec3 *b = luaL_checkudata(L, 2, "tu.vec3");
	vec3 *c = lua_newuserdatauv(L, sizeof(vec3), 0);
	*c = amat4_multvec(*a, *b);
	luaL_setmetatable(L, "tu.vec3");
	return 1;
}

static int l_amat4_rot(lua_State *L)
{
	amat4 *a = luaL_checkudata(L, 1, "tu.amat4");
	float ux = luaL_checknumber(L, 2);
	float uy = luaL_checknumber(L, 3);
	float uz = luaL_checknumber(L, 4);
	float s = luaL_checknumber(L, 5);
	float c = luaL_checknumber(L, 6);
	amat4 *tmp = lua_newuserdatauv(L, sizeof(amat4), 0);
	*tmp = amat4_rot(*a, ux, uy, uz, s, c);
	luaL_setmetatable(L, "tu.amat4");
	return 1;
}

static int l_amat4_lookat(lua_State *L)
{
	vec3 *p = luaL_checkudata(L, 1, "tu.vec3");
	vec3 *q = luaL_checkudata(L, 2, "tu.vec3");
	vec3 *u = luaL_checkudata(L, 3, "tu.vec3");
	amat4 *c = lua_newuserdatauv(L, sizeof(amat4), 0);
	*c = amat4_lookat(*p, *q, *u);
	luaL_setmetatable(L, "tu.amat4");
	return 1;
}

static int l_amat4_inverse(lua_State *L)
{
	amat4 *a = luaL_checkudata(L, 1, "tu.amat4");
	*a = amat4_inverse(*a);
	return 0;
}

static int l_amat4_inversed(lua_State *L)
{
	amat4 *a = luaL_checkudata(L, 1, "tu.amat4");
	amat4 *c = lua_newuserdatauv(L, sizeof(amat4), 0);
	*c = amat4_inverse(*a);
	luaL_setmetatable(L, "tu.amat4");
	return 1;
}

//Pushes a onto the Lua stack as a "tu.mat4" userdata value
void l_mat4_push(lua_State *L, float a[16])
{
	float *m = lua_newuserdatauv(L, sizeof(float) * 16, 0);
	luaL_setmetatable(L, "tu.mat4");
	memcpy(m, a, sizeof(float) * 16);
}

void make_projection_matrix(float fov, float a, float n, float f, float *buf);
static int l_mat4_projection(lua_State *L)
{
	float m[16];
	float fov = luaL_checknumber(L, 1);
	float aspect = luaL_checknumber(L, 2);
	float near = luaL_checknumber(L, 3);
	float far = luaL_checknumber(L, 4);
	make_projection_matrix(fov, aspect, near, far, m);
	l_mat4_push(L, m);
	return 1;
}

void make_ortho_matrix(float l, float r, float b, float t, float n, float f, float *buf);
static int l_mat4_orthographic(lua_State *L)
{
	float m[16];
	float l = luaL_checknumber(L, 1);
	float r = luaL_checknumber(L, 2);
	float b = luaL_checknumber(L, 3);
	float t = luaL_checknumber(L, 4);
	float n = luaL_checknumber(L, 5);
	float f = luaL_checknumber(L, 6);
	make_ortho_matrix(l, r, b, t, n, f, m);
	l_mat4_push(L, m);
	return 1;
}

void make_ortho_inverse_matrix(float l, float r, float b, float t, float n, float f, float *buf);
static int l_mat4_orthographic_inverse(lua_State *L)
{
	float m[16];
	float l = luaL_checknumber(L, 1);
	float r = luaL_checknumber(L, 2);
	float b = luaL_checknumber(L, 3);
	float t = luaL_checknumber(L, 4);
	float n = luaL_checknumber(L, 5);
	float f = luaL_checknumber(L, 6);
	make_ortho_inverse_matrix(l, r, b, t, n, f, m);
	l_mat4_push(L, m);
	return 1;
}
static 

int l_amat4__tostring(lua_State *L)
{
	amat4 *a = luaL_checkudata(L, 1, "tu.amat4");
	luaL_Buffer b;
	luaL_buffinit(L, &b);
#define PUSHMATVAL(i, sep) lua_pushnumber(L, amat4_index(*a, i)); luaL_addvalue(&b); luaL_addstring(&b, sep);
	PUSHMATVAL(0, ", ")
	PUSHMATVAL(1, ", ")
	PUSHMATVAL(2, ", ")
	PUSHMATVAL(3, ",\n")
	PUSHMATVAL(4, ", ")
	PUSHMATVAL(5, ", ")
	PUSHMATVAL(6, ", ")
	PUSHMATVAL(7, ",\n")
	PUSHMATVAL(8, ", ")
	PUSHMATVAL(9, ", ")
	PUSHMATVAL(10, ", ")
	PUSHMATVAL(11, ",\n")
	PUSHMATVAL(12, ", ")
	PUSHMATVAL(13, ", ")
	PUSHMATVAL(14, ", ")
	PUSHMATVAL(15, "\n")
#undef PUSHMATVAL
	luaL_pushresult(&b);
	return 1;
}

int l_mat4__tostring(lua_State *L)
{
	float *m = luaL_checkudata(L, 1, "tu.mat4");
	luaL_Buffer b;
	luaL_buffinit(L, &b);
#define PUSHMATVAL(i, sep) lua_pushnumber(L, m[i]); luaL_addvalue(&b); luaL_addstring(&b, sep);
	PUSHMATVAL(0, ", ")
	PUSHMATVAL(1, ", ")
	PUSHMATVAL(2, ", ")
	PUSHMATVAL(3, ",\n")
	PUSHMATVAL(4, ", ")
	PUSHMATVAL(5, ", ")
	PUSHMATVAL(6, ", ")
	PUSHMATVAL(7, ",\n")
	PUSHMATVAL(8, ", ")
	PUSHMATVAL(9, ", ")
	PUSHMATVAL(10, ", ")
	PUSHMATVAL(11, ",\n")
	PUSHMATVAL(12, ", ")
	PUSHMATVAL(13, ", ")
	PUSHMATVAL(14, ", ")
	PUSHMATVAL(15, "\n")
#undef PUSHMATVAL
	luaL_pushresult(&b);
	return 1;
}

int l_mat4__index(lua_State *L)
{
	float *m = luaL_checkudata(L, 1, "tu.mat4");
	lua_Integer n = luaL_checkinteger(L, 2);
	if (n < 0 || n > 15) {
		luaL_error(L, "Index must be 0-15 (inclusive)");
		return 0;
	}
	lua_pushnumber(L, m[n]);
	return 2;
}

/*
Functions I did not bother to wrap:

//Replace these with a __tostring metamethod, taking an optional format parameter.
//Prints a vec3 like so: "{x, y, z}" (no newline).
void vec3_print(vec3 a);
//Prints a vec3 like so: "{x, y, z}" (with newline).
void vec3_println(vec3 a);
//Prints a vec3 like so: "{x, y, z}" (no newline). Takes a printf format for printing each float.
void vec3_printf(char *fmt, vec3 a);
//Create a new mat3 from an array of floats. Row-major order.
mat3 mat3_from_array(float *array) __attribute__ ((const));
//3x3 Identity matrix.
mat3 mat3_ident() __attribute__ ((const));
//Produces the matrix for a rotation about <ux, uy, uz> by some angle, provided by s and c.
//s and c should be the sine and cosine of the angle, respectively.
mat3 mat3_rotmat(float ux, float uy, float uz, float s, float c) __attribute__ ((const));
//Produces a rotation matrix about the three basis vectors by some angle, provided by s and c.
//s and c should be the sine and cosine of the angle, respectively.
mat3 mat3_rotmatx(float s, float c) __attribute__ ((const));
mat3 mat3_rotmaty(float s, float c) __attribute__ ((const));
mat3 mat3_rotmatz(float s, float c) __attribute__ ((const));
//Copy a into a buffer representing a true 3x3 row-major matrix.
//The buffer should be large enough to store 9 floats.
void mat3_to_array(mat3 a, float *buf);
//Copy a into a buffer representing a true 3x3 column-major matrix.
//The buffer should be large enough to store 9 floats.
void mat3_to_array_cm(mat3 a, float *buf);
//Takes a mat3 and a vec3, and copies them into a buffer representing a true, row-major 4x4 matrix.
//a becomes the rotation portion, and b becomes the translation.
void mat3_vec3_to_array(mat3 a, vec3 b, float *buf) __attribute__ ((const));
//Prints a mat3, row by row.
void mat3_print(mat3 a);
//Prints an array of 9 floats, row by row.
void mat3_buf_print(float *a);
//Copy a into a buffer representing a true 4x4 row-major matrix.
//The buffer should be large enough to store 16 floats.
//The last row will be <0, 0, 0, 1>.
void amat4_to_array(amat4 a, float *buf);
//Produce a rotation matrix which represents a rotation about <ux, uy, uz> by some angle.
//s and c should be the sine and cosine of the angle, respectively.
amat4 amat4_rotmat(float ux, float uy, float uz, float s, float c) __attribute__ ((const));
//Produce a rotation matrix which represents a rotation about <ux, uy, uz> by angle, in radians.
//This version tries to reduce multiplications at the cost of more interdependant local variables.
amat4 amat4_rotmat_lomult(float ux, float uy, float uz, float s, float c) __attribute__ ((const));
//Multiply the 4x4 row-major matrix a by the column vector b and return a new vector.
//b is implied to be a 4-vec with the form <x, y, z, 1>
//a should be an array of 16 floats.
vec3 amat4_buf_multpoint(float *a, float *b, float *out);
*/

static luaL_Reg lua_glla[] = {
	{"vec2", l_vec2_new},
	{"vec3", NULL},
	{"vec4", NULL},
	{"mat3", NULL}, //mat3, mat4 and amat4 will be implemented as callable tables
	{"mat4", NULL}, //this makes it possible to do something like glla.mat3.identity()
	{"amat4", NULL}, //without actually creating a mat3 instance every time
	{NULL, NULL}
};

static luaL_Reg l_vec3[] = {
	{"__call", l_vec3_new},
	{"__index", l_vec3__index},
	{"__newindex", l_vec3__newindex},
	{"__tostring", l_vec3__tostring},
	{"__add", l_vec3_add},
	{"__sub", l_vec3_sub},
	{"__mul", l_vec3_mul},
	{"__div", l_vec3_div},
	{"normalize", l_vec3_normalize},
	{"normalized", l_vec3_normalized},
	{"cross", l_vec3_cross},
	{"lerp", l_vec3_lerp},
	{"dot", l_vec3_dot},
	{"mag", l_vec3_mag},
	{"dist", l_vec3_dist},
	{NULL, NULL}
};

static luaL_Reg l_vec4[] = {
	{"__call", l_vec4_new},
	{"__index", l_vec4__index},
	{"__newindex", l_vec4__newindex},
	{"__tostring", l_vec4__tostring},
	{"__add", l_vec4_add},
	{"__sub", l_vec4_sub},
	{"__mul", l_vec4_mul},
	{"__div", l_vec4_div},
	{NULL, NULL}
};

static luaL_Reg l_mat3[] = {
	{"__call", l_mat3_new},
	{"__index", l_mat3__index},
	{"__newindex", l_mat3__newindex},
	{"__mul", l_mat3__mul},
	{"multvec", l_mat3_multvec},
	{"rot", l_mat3_rot},
	{"scale", l_mat3_scale},
	{"transpose", l_mat3_transpose},
	{"transposed", l_mat3_transposed},
	{"lookat", l_mat3_lookat},
	{"identity", l_mat3_identity},
	{"scalemat", l_mat3_scalemat},
	{NULL, NULL}
};

static luaL_Reg l_amat4[] = {
	{"__call", l_amat4_new},
	{"__index", l_amat4__index},
	{"__newindex", l_amat4__newindex},
	{"__mul", l_amat4_mult},
	{"__tostring", l_amat4__tostring},
	//TODO: Roll one of these into l_amat4_mult?
	{"multvec", l_amat4_multvec},
	{"multpoint", l_amat4_multpoint},
	{"rot", l_amat4_rot},
	{"lookat", l_amat4_lookat},
	{"identity", l_amat4_identity},
	{"inverse", l_amat4_inverse},
	{"inversed", l_amat4_inversed},
	{NULL, NULL}
};

static luaL_Reg l_mat4[] = {
	{"__mul", l_amat4_mult},
	{"__index", l_mat4__index},
	{"__tostring", l_mat4__tostring},
	{"multpoint", l_mat4_multpoint},
	{"projection", l_mat4_projection},
	{"orthographic", l_mat4_orthographic},
	{"orthographic_inverse", l_mat4_orthographic_inverse},
	{NULL, NULL}
};

int luaopen_l_glla(lua_State *L)
{
	luaL_newmetatable(L, "tu.vec2");
	luaL_newlib(L, lua_glla);

	luaL_newmetatable(L, "tu.vec3");
	luaL_setfuncs(L, l_vec3, 0);
	lua_createtable(L, 0, 1);
	lua_pushcfunction(L, l_vec3_new);
	lua_setfield(L, -2, "__call");
	lua_setmetatable(L, -2);
	lua_setfield(L, -2, "vec3");

	luaL_newmetatable(L, "tu.mat3");
	luaL_setfuncs(L, l_mat3, 0);
	lua_createtable(L, 0, 1);
	lua_pushcfunction(L, l_mat3_new);
	lua_setfield(L, -2, "__call");
	lua_setmetatable(L, -2);
	lua_setfield(L, -2, "mat3");

	luaL_newmetatable(L, "tu.mat4");
	luaL_setfuncs(L, l_mat4, 0);
	lua_setfield(L, -2, "mat4");

	luaL_newmetatable(L, "tu.amat4");
	luaL_setfuncs(L, l_amat4, 0);
	lua_createtable(L, 0, 1);
	lua_pushcfunction(L, l_amat4_new);
	lua_setfield(L, -2, "__call");
	lua_setmetatable(L, -2);
	lua_setfield(L, -2, "amat4");

	luaL_newmetatable(L, "tu.vec4");
	luaL_setfuncs(L, l_vec4, 0);
	lua_createtable(L, 0, 1);
	lua_pushcfunction(L, l_vec4_new);
	lua_setfield(L, -2, "__call");
	lua_setmetatable(L, -2);
	lua_setfield(L, -2, "vec4");

	return 1;
}

// 