//Lua headers
#include <lua-5.4.4/src/lua.h>
#include <lua-5.4.4/src/lauxlib.h>
#include <lua-5.4.4/src/lualib.h>
#include <stdlib.h>
//Graphics header includes OpenGL
#include "graphics.h"
#include "glla.h"
#include "macros.h"
#include "scripts/lib/debug/debugger_lua.h"

struct tu_gluint_array
{
	size_t num;
	GLuint array[0];
};

struct tu_gluint_array_ref
{
	int ref; //From luaL_ref, points to the VertexArrays object this array is part of.
	GLuint value;
};

static int l_glActiveTexture(lua_State *L)
{
	glActiveTexture((GLenum)luaL_checkinteger(L, 1)); //texture
	return 0;
}

static int l_Buffers(lua_State *L)
{
	GLsizei n = (GLsizei)luaL_checkinteger(L, 1);
	size_t size = sizeof(struct tu_gluint_array) + sizeof(GLuint) * n;
	struct tu_gluint_array *buffers = lua_newuserdatauv(L, size, 0);
	memset(buffers, 0, size);
	buffers->num = n;
	luaL_setmetatable(L, "tu.gl.Buffers");
	glGenBuffers(n, buffers->array);
	return 1;
}

static int l_Buffers__gc(lua_State *L)
{
	struct tu_gluint_array *buffers = luaL_checkudata(L, 1, "tu.gl.Buffers");
	// printf("Buffers being collected\n");
	glDeleteBuffers(buffers->num, buffers->array);
	return 0;
}

static int l_Buffers__index(lua_State *L)
{
	struct tu_gluint_array *buffers = luaL_checkudata(L, 1, "tu.gl.Buffers");
	lua_Integer i = luaL_checkinteger(L, 2);
	if (i > 0 && i <= buffers->num) {
		struct tu_gluint_array_ref *buffer = lua_newuserdatauv(L, sizeof(struct tu_gluint_array_ref), 0);
		//TODO: Carefully review the order of these statements.
		luaL_getmetatable(L, "tu.gl.Buffer");
		lua_pushvalue(L, 1);
		buffer->ref = luaL_ref(L, -2); //buffers will now not be collected
		// printf("ref-ing Buffer %i\n", buffer->ref);
		buffer->value = buffers->array[i-1];
		lua_setmetatable(L, -2); //buffer will now unref on __gc
	}
	else {
		lua_pushnil(L);
	}
	return 1;
}

static int l_Buffers__len(lua_State *L)
{
	struct tu_gluint_array *buffers = luaL_checkudata(L, 1, "tu.gl.Buffers");
	lua_pushinteger(L, buffers->num);
	return 1;
}

static int l_Buffer__gc(lua_State *L)
{
	struct tu_gluint_array_ref *buffer = luaL_checkudata(L, 1, "tu.gl.Buffer");
	lua_getmetatable(L, -1);
	// printf("unref-ing Buffer %i\n", buffer->ref);
	luaL_unref(L, -1, buffer->ref);
	return 0;
}

static int l_glBindBuffer(lua_State *L)
{
	GLenum *buffertarget = luaL_checkudata(L, 1, "tu.gl.BufferTarget");
	struct tu_gluint_array_ref *buffer = luaL_checkudata(L, 2, "tu.gl.Buffer");
	glBindBuffer(*buffertarget, buffer->value);
	return 0;
}

/*
Approaches for passing Lua tables in as buffers 2021/07/08

1. Have an "attribute pointer mapping" table, essentially what's built up by calls to glVertexAttribPointer.
This could be an actual table or an object built up through successive calls. Once constructed, it is used to
interpret a Lua table as a buffer, value-by-value, casting to the expected size.

2. Put expected vertex attributes into the keys of a table. Their "type" is set through various constructor functions.
{
	color = vec3(),
	time = float(),
}

Vertex buffers *could be* tables of these vertex attribute tables (slow with string keys)
{
	{color = vec3(1,0,0), time = 10},
	{color = vec3(0,1,0), time = 10},
}
Having just arrays of numbers is faster, the issue is figuring out what order the attributes should be in.
{1, 0, 0, 10, 0, 1, 0, 10}
Including the name as a member of the attribute pointer entry solves this problem but makes the code look a bit messier.
{
	{color = vec3()},
	{time = float()},
}
We need to specify how to interpret the vertex data *somewhere*.
An array of structs in C is nice because it allows named AND ordered members with a clean syntax and no overhead.

3. Create a C object that's indexable by Lua to hold a vertex buffer.
This is very fast and can be compatible with vertex buffers described in C.
Unfortunately you wouldn't be able to take advantage of Lua's table constructor syntax
(without a conversion and an attribute pointer mapping)

2022/10/01

Using constructor functions:

VertexFormat {
	gl.vec3 'color',
	gl.float 'time',
}

Could be something like:

local buffers = gl.Buffers(1)
buffers:bind:bufferData
*/

static int l_glPack32f(lua_State *L)
{
	size_t len = 0;
	int top = lua_gettop(L);
	for (int i = 1; i <= top; i++) {
		luaL_checktype(L, i, LUA_TTABLE);
		len += luaL_len(L, i);
	}
	float *data = lua_newuserdatauv(L, sizeof(float) * len, 1);
	if (!data)
		luaL_error(L, "Allocation failed!");
	luaL_setmetatable(L, "tu.gl.PackedBuffer");
	lua_pushinteger(L, sizeof(float) * len);
	lua_setiuservalue(L, -2, 1);
	// printf("Packing %lu elements into a float buffer\n", len);

	float *p = data;
	for (int i = 1; i <= top; i++) {
		size_t tlen = luaL_len(L, i);
		for (int j = 1; j <= tlen; j++) {
			lua_geti(L, i, j);
			*p = lua_tonumber(L, -1);
			p++;
			lua_pop(L, 1);
		}
	}
	return 1;
}

static int l_glPack32i(lua_State *L)
{
	size_t len = 0;
	int top = lua_gettop(L);
	for (int i = 1; i <= top; i++) {
		luaL_checktype(L, i, LUA_TTABLE);
		len += luaL_len(L, i);
	}
	uint32_t *data = lua_newuserdatauv(L, sizeof(uint32_t) * len, 1);
	if (!data)
		luaL_error(L, "Allocation failed!");
	luaL_setmetatable(L, "tu.gl.PackedBuffer");
	lua_pushinteger(L, sizeof(uint32_t) * len);
	lua_setiuservalue(L, -2, 1);
	// printf("Packing %lu elements into an int buffer\n", len);

	uint32_t *p = data;
	for (int i = 1; i <= top; i++) {
		size_t tlen = luaL_len(L, i);
		for (int j = 1; j <= tlen; j++) {
			lua_geti(L, i, j);
			*p = lua_tointeger(L, -1);
			p++;
			lua_pop(L, 1);
		}
	}
	return 1;
}

static int l_glBufferData(lua_State *L)
{
	lua_Integer size = luaL_checkinteger(L, 2);
	const void *data = luaL_checkudata(L, 3, "tu.gl.PackedBuffer");
	lua_getiuservalue(L, 3, 1);
	lua_Integer packed_size = lua_tointeger(L, -1);
	if (packed_size < size)
		return luaL_error(L, "Trying to copy a larger number of bytes than size of buffer");

	// printf("Buffering %lld bytes (%lld packed)\n", size, packed_size);
	glBufferData(
		*(GLenum *)luaL_checkudata(L, 1, "tu.gl.BufferTarget"),
		size,
		data,
		*(GLenum *)luaL_checkudata(L, 4, "tu.gl.BufferUsage"));
	return 0;
}

static int l_glBindTexture(lua_State *L)
{
	glBindTexture((GLenum)luaL_checkinteger(L, 1), //target
		(GLuint)luaL_checkinteger(L, 2)); //texture
	return 0;
}

static int l_vertexArrays(lua_State *L)
{
	GLsizei n = (GLsizei)luaL_checkinteger(L, 1);
	size_t size = sizeof(struct tu_gluint_array) + sizeof(GLuint) * n;
	struct tu_gluint_array *arrays = lua_newuserdatauv(L, size, 0);
	memset(arrays, 0, size);
	arrays->num = n;
	luaL_setmetatable(L, "tu.gl.VertexArrays");
	glGenVertexArrays(n, arrays->array);
	return 1;
}

static int l_VertexArrays__gc(lua_State *L)
{
	struct tu_gluint_array *arrays = luaL_checkudata(L, 1, "tu.gl.VertexArrays");
	// printf("VertexArrays being collected\n");
	glDeleteVertexArrays(arrays->num, arrays->array);
	return 0;
}

static int l_VertexArrays__index(lua_State *L)
{
	struct tu_gluint_array *arrays = luaL_checkudata(L, 1, "tu.gl.VertexArrays");
	lua_Integer i = luaL_checkinteger(L, 2);
	if (i > 0 && i <= arrays->num) {
		struct tu_gluint_array_ref *array = lua_newuserdatauv(L, sizeof(struct tu_gluint_array_ref), 0);
		//TODO: Carefully review the order of these statements.
		luaL_getmetatable(L, "tu.gl.VertexArray");
		lua_pushvalue(L, 1);
		array->ref = luaL_ref(L, -2); //arrays will now not be collected
		// printf("ref-ing VertexArray %i\n", array->ref);
		array->value = arrays->array[i-1];
		lua_setmetatable(L, -2); //array will now unref on __gc
	}
	else {
		lua_pushnil(L);
	}
	return 1;
}

static int l_VertexArrays__len(lua_State *L)
{
	struct tu_gluint_array *arrays = luaL_checkudata(L, 1, "tu.gl.VertexArrays");
	lua_pushinteger(L, arrays->num);
	return 1;
}

static int l_VertexArray__gc(lua_State *L)
{
	struct tu_gluint_array_ref *array = luaL_checkudata(L, 1, "tu.gl.VertexArray");
	lua_getmetatable(L, -1);
	// printf("unref-ing VertexArray %i\n", array->ref);
	luaL_unref(L, -1, array->ref);
	return 0;
}

static int l_VertexArray__tostring(lua_State *L)
{
	struct tu_gluint_array_ref *array = luaL_checkudata(L, 1, "tu.gl.VertexArray");
	luaL_Buffer b;
	luaL_buffinit(L, &b);
	lua_pushnumber(L, array->value);
	luaL_addvalue(&b);
	luaL_pushresult(&b);
	return 1;
}

static int l_glBindVertexArray(lua_State *L)
{
	glBindVertexArray(((struct tu_gluint_array_ref *)luaL_checkudata(L, 1, "tu.gl.VertexArray"))->value);
	return 0;
}

static int l_glClear(lua_State *L)
{
	glClear((GLbitfield)luaL_checkinteger(L, 1)); //mask
	return 0;
}

static int l_glClearColor(lua_State *L)
{
	glClearColor(
		(GLclampf)luaL_checknumber(L, 1), //red
		(GLclampf)luaL_checknumber(L, 2), //green
		(GLclampf)luaL_checknumber(L, 3), //blue
		(GLclampf)luaL_checknumber(L, 4)); //alpha
	return 0;
}

static int l_glClearDepth(lua_State *L)
{
	glClearDepth((GLclampd)luaL_checknumber(L, 1)); //depth
	return 0;
}

static int l_glDeleteShader(lua_State *L)
{
	GLuint *shader = luaL_checkudata(L, 1, "tu.Shader");
	// printf("l_glDeleteShader was called on shader %i!\n", *shader);
	glDeleteShader(*shader);
	return 0;
}

static int l_glDeleteProgram(lua_State *L)
{
	GLuint *program = luaL_checkudata(L, 1, "tu.ShaderProgram");
	// printf("l_glDeleteProgram was called on shader program %i!\n", *program);
	glDeleteProgram(*program);
	return 0;
}

static int l_glDepthFunc(lua_State *L)
{
	glDepthFunc(*(GLenum *)luaL_checkudata(L, 1, "tu.gl.DepthFunc"));
	return 0;
}

static int l_glDisable(lua_State *L)
{
	GLenum *enableval = luaL_checkudata(L, 1, "tu.gl.EnableValue");
	glDisable(*enableval);
	return 0;
}

static int l_glDrawArraysInstanced(lua_State *L)
{
	glDrawArraysInstanced(
		*(GLenum *)luaL_checkudata(L, 1, "tu.gl.PrimitiveMode"),
		(GLint)luaL_checkinteger(L, 2),//first
		(GLsizei)luaL_checkinteger(L, 3),//count
		(GLsizei)luaL_checkinteger(L, 4)); //instancecount
	return 0;
}

static int l_glDrawElementsInstanced(lua_State *L)
{
	glDrawElementsInstanced(
		*(GLenum *)luaL_checkudata(L, 1, "tu.gl.PrimitiveMode"),
		(GLsizei)luaL_checkinteger(L, 2),//count
		*(GLenum *)luaL_checkudata(L, 3, "tu.gl.PointerType"),//type
		(const void *)luaL_checkinteger(L, 4),//indices
		(GLsizei)luaL_checkinteger(L, 5)); //instancecount
	return 0;
}

static int l_glDrawElementsInstancedBaseVertex(lua_State *L)
{
	glDrawElementsInstancedBaseVertex(
		*(GLenum *)luaL_checkudata(L, 1, "tu.gl.PrimitiveMode"),
		(GLsizei)luaL_checkinteger(L, 2),//count
		*(GLenum *)luaL_checkudata(L, 3, "tu.gl.PointerType"),//type
		(const void *)luaL_checkinteger(L, 4),//indices
		(GLsizei)luaL_checkinteger(L, 5), //instancecount
		(GLint)luaL_checkinteger(L, 6)); //basevertex
	return 0;
}

static int l_glEnable(lua_State *L)
{
	GLenum *enableval = luaL_checkudata(L, 1, "tu.gl.EnableValue");
	glEnable(*enableval);
	return 0;
}

static int l_glEnableVertexAttribArray(lua_State *L)
{
	glEnableVertexAttribArray((GLuint)luaL_checkinteger(L, 1)); //index
	return 0;
}

static int l_glGetAttribLocation(lua_State *L)
{
	GLuint *program = luaL_checkudata(L, 1, "tu.ShaderProgram");
	lua_pushinteger(L, glGetAttribLocation(*program,
		(const GLchar*)luaL_checkstring(L, 2)));
	return 1;
}

static int l_glGetUniformLocation(lua_State *L)
{
	GLuint *program = luaL_checkudata(L, 1, "tu.ShaderProgram");
	lua_pushinteger(L, glGetUniformLocation(*program,
		(const GLchar*)luaL_checkstring(L, 2)));
	return 1;
}

static int l_glPolygonMode(lua_State *L)
{
	GLenum *mode = luaL_checkudata(L, 1, "tu.gl.PolygonMode");
	glPolygonMode(GL_FRONT_AND_BACK, *mode);
	return 0;
}

static int l_glPrimitiveRestartIndex(lua_State *L)
{
	glPrimitiveRestartIndex((GLuint)luaL_checkinteger(L, 1)); //index
	return 0;
}

static int l_glUseProgram(lua_State *L)
{
	GLuint *program = luaL_checkudata(L, 1, "tu.ShaderProgram");
	glUseProgram(*program);
	lua_pushvalue(L, 1); //Just in case additional arguments were passed
	lua_setfield(L, LUA_REGISTRYINDEX, "tu_activeShaderProgram");
	return 0;
}

static int l_glVertexAttribDivisor(lua_State *L)
{
	glVertexAttribDivisor((GLuint)luaL_checkinteger(L, 1), //index
		(GLuint)luaL_checkinteger(L, 2)); //divisor
	return 0;
}

static int l_glVertexAttribPointer(lua_State *L)
{
	if (!lua_isboolean(L, 4))
		return luaL_argerror(L, 4, "Argument must be a boolean");
	glVertexAttribPointer(
		(GLint)luaL_checkinteger(L, 1), //index
		(GLint)luaL_checkinteger(L, 2), //size
		*(GLenum *)luaL_checkudata(L, 3, "tu.gl.PointerType"), //type,
		(GLboolean)lua_toboolean(L, 4), //normalized,
		(GLsizei)luaL_checkinteger(L, 5), //stride
		(const void *)luaL_checkinteger(L, 6)); //pointer
	return 0;
}

static int l_glVertexAttribIPointer(lua_State *L)
{
	glVertexAttribIPointer(
		(GLint)luaL_checkinteger(L, 1), //index
		(GLint)luaL_checkinteger(L, 2), //size
		*(GLenum *)luaL_checkudata(L, 3, "tu.gl.PointerType"), //type,
		(GLsizei)luaL_checkinteger(L, 4), //stride
		(const void *)luaL_checkinteger(L, 5)); //pointer
	return 0;
}


static int l_glViewport(lua_State *L)
{
	glViewport((GLint)luaL_checkinteger(L, 1), (GLint)luaL_checkinteger(L, 2),
		(GLsizei)luaL_checkinteger(L, 3), (GLsizei)luaL_checkinteger(L, 4));
	return 0;
}

static int l_Framebuffers(lua_State *L)
{
	GLsizei n = (GLsizei)luaL_checkinteger(L, 1);
	size_t size = sizeof(struct tu_gluint_array) + sizeof(GLuint) * n;
	struct tu_gluint_array *framebuffers = lua_newuserdatauv(L, size, 0);
	memset(framebuffers, 0, size);
	framebuffers->num = n;
	luaL_setmetatable(L, "tu.gl.Framebuffers");
	glGenFramebuffers(n, framebuffers->array);
	return 1;
}

static int l_Framebuffers__gc(lua_State *L)
{
	struct tu_gluint_array *framebuffers = luaL_checkudata(L, 1, "tu.gl.Framebuffers");
	// printf("Framebuffers being collected\n");
	glDeleteFramebuffers(framebuffers->num, framebuffers->array);
	return 0;
}

static int l_Framebuffers__index(lua_State *L)
{
	struct tu_gluint_array *framebuffers = luaL_checkudata(L, 1, "tu.gl.Framebuffers");
	lua_Integer i = luaL_checkinteger(L, 2);
	if (i > 0 && i <= framebuffers->num) {
		struct tu_gluint_array_ref *framebuffer = lua_newuserdatauv(L, sizeof(struct tu_gluint_array_ref), 0);
		//TODO: Carefully review the order of these statements.
		luaL_getmetatable(L, "tu.gl.Framebuffer");
		lua_pushvalue(L, 1);
		framebuffer->ref = luaL_ref(L, -2); //framebuffers will now not be collected
		// printf("ref-ing Buffer %i\n", framebuffer->ref);
		framebuffer->value = framebuffers->array[i-1];
		lua_setmetatable(L, -2); //framebuffer will now unref on __gc
	}
	else {
		lua_pushnil(L);
	}
	return 1;
}

static int l_Framebuffers__len(lua_State *L)
{
	struct tu_gluint_array *framebuffers = luaL_checkudata(L, 1, "tu.gl.Framebuffers");
	lua_pushinteger(L, framebuffers->num);
	return 1;
}

static int l_Framebuffer__gc(lua_State *L)
{
	struct tu_gluint_array_ref *framebuffer = luaL_checkudata(L, 1, "tu.gl.Framebuffer");
	lua_getmetatable(L, -1);
	// printf("unref-ing Framebuffer %i\n", framebuffer->ref);
	luaL_unref(L, -1, framebuffer->ref);
	return 0;
}

static int l_glBindFramebuffer(lua_State *L) {
	GLenum *target = luaL_checkudata(L, 1, "tu.gl.FramebufferTarget");
    struct tu_gluint_array_ref *fb = luaL_checkudata(L, 2, "tu.gl.Framebuffer");
    glBindFramebuffer(*target, fb->value);
    return 0;
}

////////////////
static int l_Textures(lua_State *L)
{
	GLsizei n = (GLsizei)luaL_checkinteger(L, 1);
	size_t size = sizeof(struct tu_gluint_array) + sizeof(GLuint) * n;
	struct tu_gluint_array *textures = lua_newuserdatauv(L, size, 0);
	memset(textures, 0, size);
	textures->num = n;
	luaL_setmetatable(L, "tu.gl.Textures");
	glGenTextures(n, textures->array);
	return 1;
}

static int l_Textures__gc(lua_State *L)
{
	struct tu_gluint_array *textures = luaL_checkudata(L, 1, "tu.gl.Textures");
	// printf("Textures being collected\n");
	glDeleteTextures(textures->num, textures->array);
	return 0;
}

static int l_Textures__index(lua_State *L)
{
	struct tu_gluint_array *textures = luaL_checkudata(L, 1, "tu.gl.Textures");
	lua_Integer i = luaL_checkinteger(L, 2);
	if (i > 0 && i <= textures->num) {
		struct tu_gluint_array_ref *texture = lua_newuserdatauv(L, sizeof(struct tu_gluint_array_ref), 0);
		//TODO: Carefully review the order of these statements.
		luaL_getmetatable(L, "tu.gl.Texture");
		lua_pushvalue(L, 1);
		texture->ref = luaL_ref(L, -2); //textures will now not be collected
		// printf("ref-ing Buffer %i\n", texture->ref);
		texture->value = textures->array[i-1];
		lua_setmetatable(L, -2); //texture will now unref on __gc
	}
	else {
		lua_pushnil(L);
	}
	return 1;
}

static int l_Textures__len(lua_State *L)
{
	struct tu_gluint_array *textures = luaL_checkudata(L, 1, "tu.gl.Textures");
	lua_pushinteger(L, textures->num);
	return 1;
}

static int l_Texture__gc(lua_State *L)
{
	struct tu_gluint_array_ref *texture = luaL_checkudata(L, 1, "tu.gl.Texture");
	lua_getmetatable(L, -1);
	// printf("unref-ing Texture %i\n", texture->ref);
	luaL_unref(L, -1, texture->ref);
	return 0;
}

// static int l_glBindTexture(lua_State *L) {
// 	GLenum *target = luaL_checkudata(L, 1, "tu.gl.TextureTarget");
//     struct tu_gluint_array_ref *fb = luaL_checkudata(L, 2, "tu.gl.Texture");
//     glBindTexture(*target, fb->value);
//     return 0;
// }

#if 0

// Assume you have a userdata wrapper for texture
typedef struct {
    GLuint id;
} Texture;

static int l_gen_textures(lua_State *L) {
    GLuint texture;
    glGenTextures(1, &texture);

    // Create and push the Texture userdata
    Texture *tex = (Texture *)lua_newuserdata(L, sizeof(Texture));
    tex->id = texture;

    // Set metatable for the userdata (you may need to define the metatable elsewhere)
    luaL_setmetatable(L, "Texture");

    return 1;
}

static Texture *check_texture(lua_State *L, int index) {
    return (Texture *)luaL_checkudata(L, index, "Texture");
}

static int l_bind_texture(lua_State *L) {
    Texture *tex = check_texture(L, 1);
    glBindTexture(GL_TEXTURE_2D, tex->id);
    return 0;
}

static int l_tex_image_2d(lua_State *L) {
    GLenum target = (GLenum)luaL_checkinteger(L, 1);
    GLint level = (GLint)luaL_checkinteger(L, 2);
    GLint internalformat = (GLint)luaL_checkinteger(L, 3);
    GLsizei width = (GLsizei)luaL_checkinteger(L, 4);
    GLsizei height = (GLsizei)luaL_checkinteger(L, 5);
    GLint border = (GLint)luaL_checkinteger(L, 6);
    GLenum format = (GLenum)luaL_checkinteger(L, 7);
    GLenum type = (GLenum)luaL_checkinteger(L, 8);
    const void *pixels = luaL_checkstring(L, 9); // Assuming pixels are passed as a string

    glTexImage2D(target, level, internalformat, width, height, border, format, type, pixels);

    return 0;
}

#endif

static int l_glGetError(lua_State *L)
{
	GLenum errorcode = glGetError();
	switch (errorcode) {
	case GL_INVALID_ENUM: lua_pushliteral(L, "GL_INVALID_ENUM"); break;
	case GL_INVALID_VALUE: lua_pushliteral(L, "GL_INVALID_VALUE"); break;
	case GL_INVALID_OPERATION: lua_pushliteral(L, "GL_INVALID_OPERATION"); break;
	// case GL_STACK_OVERFLOW: lua_pushliteral(L, "GL_STACK_OVERFLOW"); break;
	// case GL_STACK_UNDERFLOW: lua_pushliteral(L, "GL_STACK_UNDERFLOW"); break;
	case GL_OUT_OF_MEMORY: lua_pushliteral(L, "GL_OUT_OF_MEMORY"); break;
	default: lua_pushinteger(L, glGetError());
	}
	return 1;
}

/* Opinionated Lua convenience wrappers */

struct color_buffer {
	GLuint fbo;
	GLuint texture;
	GLuint depth;
};

// static int l_color_buffer_new(lua_State *L)
// {
// 	lua_Integer width = luaL_checkinteger(L, 1);
// 	lua_Integer height = luaL_checkinteger(L, 2);

// 	//todo:
// 	//if first arg is a table, pull all the config values out of it
// 	//if first arg is a number, assume they just want default with provided size
// 	//if second arg is nil, assume they want square

// 	//store the "colorbuffer" in a userdata, provide methods on it?

// 	struct color_buffer tmp;
// 	glGenFramebuffers(1, &tmp.fbo);
// 	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, tmp.fbo);

// 	glGenTextures(1, &tmp.texture);
// 	glGenTextures(1, &tmp.depth);
	
// 	glBindTexture(GL_TEXTURE_2D, tmp.texture);
// 	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, NULL);
// 	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
// 	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
// 	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tmp.texture, 0);

// 	glBindTexture(GL_TEXTURE_2D, tmp.depth);
// 	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
// 	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, tmp.depth, 0);

// 	glDrawBuffer(GL_COLOR_ATTACHMENT0);

// 	GLenum error = glCheckFramebufferStatus(GL_FRAMEBUFFER);

// 	if (error != GL_FRAMEBUFFER_COMPLETE) {
// 		printf("FB error, status: 0x%x\n", error);
// 	}

// 	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
// 	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

// 	return tmp;
// }

extern char *shader_enum_to_string(GLenum shader_type);

//Pushes the log string onto the Lua stack [-0, +1, e]
static void l_pushLog(lua_State *L, GLuint handle, GLboolean is_program)
{
	//Why write two log printing functions when you can use FUNCTION POINTERS AND TERNARY OPERATORS >:D
	GLboolean (*glIs)(GLuint)                                  = is_program? glIsProgram         : glIsShader;
	void (*glGetiv)(GLuint, GLenum, GLint *)                   = is_program? glGetProgramiv      : glGetShaderiv;
	void (*glGetInfoLog)(GLuint, GLsizei, GLsizei *, GLchar *) = is_program? glGetProgramInfoLog : glGetShaderInfoLog;

	if (glIs(handle)) {
		int log_length = 0, max_length = 0;
		glGetiv(handle, GL_INFO_LOG_LENGTH, &max_length);
		char *info_log = (char *)malloc(max_length);
		glGetInfoLog(handle, max_length, &log_length, info_log);
		if (log_length > 0) {
			lua_pushlstring(L, info_log, log_length);
		}
		free(info_log);
	} else {
		lua_pushliteral(L, "Handle ");
		lua_pushinteger(L, handle);
		lua_pushstring(L, is_program ? " is not a program handle." : " is not a shader handle.");
		lua_concat(L, 3);
	}
}

//Returns a shader or nil, error string if there was a problem.
static int lua_shader(lua_State *L, GLenum type)
{
	size_t len = 0;
	luaL_checkstring(L, 1);
	lua_settop(L, 1); //Discard any extra args so our absolute indices below are correct
	const GLchar *string = lua_tolstring(L, 1, &len);
	if (string && len) {
		GLint success = GL_FALSE;
		GLuint *shader = lua_newuserdatauv(L, sizeof(GLuint), 2);
		*shader = 0;
		luaL_setmetatable(L, "tu.Shader");
		*shader = glCreateShader(type);

		glShaderSource(*shader, 1, &string, &(GLint){len});
		glCompileShader(*shader);
		glGetShaderiv(*shader, GL_COMPILE_STATUS, &success);
		if (success) {
			//Copy the shader source string to the top of the stack
			lua_pushvalue(L, 1);
			//Attach it as a uservalue, we'll retrieve it from lua_shaderProgram to parse stuff out
			lua_setiuservalue(L, 2, 1);
			//Also save the type
			lua_pushnumber(L, type);
			lua_setiuservalue(L, 2, 2);
			return 1; //Returns the userdata, already on the stack at position 2, which is now the top
		}

		lua_pushnil(L);
		lua_pushliteral(L, "Unable to compile ");       // 1
		lua_pushstring(L, shader_enum_to_string(type)); // 2
		lua_pushliteral(L, " ");                        // 3
		lua_pushinteger(L, *shader);                    // 4
		lua_pushliteral(L, "!\n");                      // 5
		l_pushLog(L, *shader, GL_FALSE);                // 6
		lua_pushliteral(L, "\n"ANSI_COLOR_BLUE"Shader source:\n"ANSI_COLOR_RESET); // 7
		lua_getfield(L, LUA_REGISTRYINDEX, "tu_builtin_shaderSourceError"); //
		lua_pushvalue(L, 1);                                                //
		lua_pushnil(L);                                                     //
		lua_pushvalue(L, -5);                                               //
		dbg_pcall(L, 3, 1, 0);                                              // 8
		lua_concat(L, 8);
		return 2; //Returns nil, error string

		//Would be nice to do a string replacement on each line to add line numbers.
		//Would also be nice to parse out the error line number and highlight that specific line.
		//Probably easiest to call a Lua function for that.
	}

	return 0;
}

static int lua_vertexShader(lua_State *L)
{
	return lua_shader(L, GL_VERTEX_SHADER);
}

static int lua_fragmentShader(lua_State *L)
{
	return lua_shader(L, GL_FRAGMENT_SHADER);
}

static int lua_geometryShader(lua_State *L)
{
	return lua_shader(L, GL_GEOMETRY_SHADER);
}

/*
lua_shaderProgram refactor:

copy each shader into a table that I store in the userdata for the ShaderProgram
if the uniforms have not been extracted, trying to index or set something on the ShaderProgram
will do that

other operations will do the same for attributes?

Just want to reduce the complication by 
*/

int lua_shaderProgram(lua_State *L)
{
	GLint success = GL_TRUE;
	int top = lua_gettop(L);

	GLuint *program = lua_newuserdatauv(L, sizeof(GLuint), 5);
	luaL_setmetatable(L, "tu.ShaderProgram");
	*program = glCreateProgram();

	//Since every arg should be a shader, "top" is the shader count.
	lua_createtable(L, top, 0); //Create a table to store shader strings
	lua_createtable(L, top, 0); //Create a table to store the type of each shader
	lua_newtable(L); //Create a table to store shader uniform types
	/* Stack now looks like this:
		1     | tu.Shader                (maybe vertex shader)
		...   | <other shaders>          (maybe)
		top   | tu.Shader                (maybe fragment shader)
		top+1 | tu.ShaderProgram
		top+2 | shader strings table
		top+3 | shader types table
		top+4 | shader uniforms table
	*/
	for (int i = 1; i <= top; i++) {
		GLuint *shader = luaL_checkudata(L, i, "tu.Shader");
		glAttachShader(*program, *shader);

		//Save the shader string and shader type into their respective tables, remembering 1-based indexing
		if (LUA_TSTRING != lua_getiuservalue(L, i, 1)) printf("Shader uservalue 1 is not a string!\n"); //Shader string
		lua_seti(L, top+2, i);
		if (LUA_TNUMBER != lua_getiuservalue(L, i, 2)) printf("Shader uservalue 2 is not a number!\n"); //Shader type
		lua_seti(L, top+3, i);
	}

	glLinkProgram(*program);
	glGetProgramiv(*program, GL_LINK_STATUS, &success);
	if (!success) {
		lua_pushnil(L);
		lua_pushliteral(L, "Unable to link program "); // 1
		lua_pushinteger(L, *program);                  // 2
		lua_pushliteral(L, "!\n");                     // 3
		l_pushLog(L, *program, GL_TRUE);               // 4
		lua_concat(L, 4);
		return 2;
	}

	//Set up and call function uniformsFromShaderStrings(shaderStrings, uniforms)
	lua_getfield(L, LUA_REGISTRYINDEX, "tu_builtin_uniformsFromShaderString");
	lua_pushvalue(L, top+2); //shader strings table
	lua_pushvalue(L, top+4); //uniforms table
	lua_call(L, 2, 0);
	/* Stack now looks like this:
		1     | tu.Shader                (maybe vertex shader)
		...   | <other shaders>          (maybe)
		top   | tu.Shader                (maybe fragment shader)
		top+1 | tu.ShaderProgram
		top+2 | shader strings table
		top+3 | shader types table
		top+4 | shader uniforms table
	*/
	//Iterate over the uniforms table and store the uniform locations
	lua_pushnil(L);
	while (lua_next(L, top+4) != 0) {
		if (lua_type(L, -2) == LUA_TSTRING) {
			GLint location = glGetUniformLocation(*program, lua_tostring(L, top+5));
			// printf("Setting uniforms.%s.%s.location = %i\n", lua_typename(L, lua_type(L, top+4)), lua_tostring(L, top+5), location);
			lua_pushliteral(L, "location");
			lua_pushinteger(L, location);
			lua_settable(L, top+6);
		}
		lua_pop(L, 1);
	}

	//Done with uniforms table, save it to the ShaderProgram
	lua_setiuservalue(L, top+1, 1);

	//Set up and call function attributesFromShaderStrings(shaderStrings, shaderTypes, program)
	lua_getfield(L, LUA_REGISTRYINDEX, "tu_builtin_attributesFromShaderStrings");
	lua_pushvalue(L, top+2); //shader strings table
	lua_pushvalue(L, top+3); //shader types table
	lua_pushvalue(L, top+1); //ShaderProgram
	lua_call(L, 3, 2);
	/* Stack now looks like this:
		1     | tu.Shader                (maybe vertex shader)
		...   | <other shaders>          (maybe)
		top   | tu.Shader                (maybe fragment shader)
		top+1 | tu.ShaderProgram
		top+2 | shader strings table
		top+3 | shader types table
		top+4 | sorted attributes table
		top+5 | attributes string
	*/
	//Done with all of these, save them to the ShaderProgram uservalues
	//[1] = uniforms table, [2] = strings table, [3] = types table, [4] = attributes table, [5] = attributes string
	lua_setiuservalue(L, top+1, 5);
	lua_setiuservalue(L, top+1, 4);
	lua_setiuservalue(L, top+1, 3);
	lua_setiuservalue(L, top+1, 2);

	//ShaderProgram is on the stack at top+1
	return 1;
}

#define TULUA_TFLOAT 0
#define TULUA_TINT 1
#define TULUA_TBOOL 2
#define TULUA_TVEC2 3
#define TULUA_TVEC3 4
#define TULUA_TVEC4 5
#define TULUA_TMAT3 6
#define TULUA_TMAT4 7
#define TULUA_TTEX 8
static void send_lua_value_as_uniform(lua_State *L, int value_idx, int type_idx)
{
	int top = lua_gettop(L);
	lua_pushliteral(L, "enumtype");
	lua_gettable(L, type_idx);
	lua_Integer type_enum = lua_tointeger(L, -1);
	lua_pushliteral(L, "count");
	lua_gettable(L, type_idx);
	lua_Integer count = lua_tointeger(L, -1);
	lua_pushliteral(L, "location");
	lua_gettable(L, type_idx);
	lua_Integer location = lua_tointeger(L, -1);
	lua_settop(L, top);

	void *m;
	float buf[16];
	if (count == 1) {
		switch(type_enum) {
		//TODO: Validate against the actual Lua type!
		case TULUA_TFLOAT: glUniform1f(location, lua_tonumber(L, value_idx)); break;
		case TULUA_TINT: glUniform1i(location, lua_tointeger(L, value_idx)); break;
		case TULUA_TBOOL: glUniform1i(location, lua_toboolean(L, value_idx)); break;
		case TULUA_TVEC2: glUniform2fv(location, 1, lua_touserdata(L, value_idx)); break;
		case TULUA_TVEC3: glUniform3fv(location, 1, lua_touserdata(L, value_idx)); break;
		case TULUA_TVEC4: glUniform4fv(location, 1, lua_touserdata(L, value_idx)); break;
		case TULUA_TMAT3:
			//TODO: Check if I can use the non-"_cm" version and leave transpose as 0
			mat3_to_array_cm(*(mat3 *)lua_touserdata(L, value_idx), buf);
			glUniformMatrix3fv(location, 1, 1, buf);
			break;
		case TULUA_TMAT4:
			m = lua_touserdata(L, value_idx);
			luaL_getmetatable(L, "tu.amat4");
			lua_getmetatable(L, value_idx);
			if (lua_rawequal(L, -1, -2)) { //m is amat4
				lua_pop(L, 2);
				amat4_to_array(*(amat4 *)m, buf);
				glUniformMatrix4fv(location, 1, 1, buf);
				break;
			}
			luaL_getmetatable(L, "tu.mat4");
			if (lua_rawequal(L, -1, -2)) { //b is mat4
				lua_pop(L, 3);
				glUniformMatrix4fv(location, 1, 1, m);
				break;
			}
			luaL_error(L, "Uniform is mat4, expected amat4 or mat4 value assignment");
		case TULUA_TTEX:
			// Could be a texture unit integer, or a Lua-wrapped texture object ("tu.image.texture").
			// If it's an integer, just do glActiveTexture(GL_TEXTURE0 + unit) and glUniform1i(location, unit)

			if (lua_isinteger(L, value_idx)) {
				glUniform1i(location, lua_tointeger(L, value_idx));
				break;
			}

			//If it's a "tu.image.texture" userdata, just call the "active" method to set up everything.
			if (luaL_checkudata(L, value_idx, "tu.image.texture")) {
				luaL_callmeta(L, value_idx, "active");
				glUniform1i(location, lua_tointeger(L, -1));
				lua_pop(L, 1);
				break;
			}

			break;
		default:
			return;
		}
	} else {
		//TODO: Check Lua array length, copy into local buffer, use correct glUniform*v function
	}

	lua_pushliteral(L, "value");
	lua_pushvalue(L, value_idx);
	lua_settable(L, type_idx);
}

int l_shaderProgramUniforms(lua_State *L)
{
	GLuint *shader = luaL_checkudata(L, 1, "tu.ShaderProgram");
	switch (lua_type(L, 2)) {
	//Return the assigned uniforms table. This is not a unique copy of the table!
	case LUA_TNIL: lua_getiuservalue(L, 1, 1); return 1;
	//TODO: Assign each k/v pair using appropriate glUniform function.
	case LUA_TTABLE:;
	}
	return 0;
}

int lua_shaderProgramGetAttributes(lua_State *L)
{
	lua_getiuservalue(L, lua_upvalueindex(1), 4);
	lua_getiuservalue(L, lua_upvalueindex(1), 5);
	return 2;
}

int lua_shaderProgram__index(lua_State *L)
{
	GLuint *shader = luaL_checkudata(L, 1, "tu.ShaderProgram");

	//Check if they're trying to access the "uniforms" function or a specific uniform by name
	luaL_checkstring(L, 2);
	int top = lua_gettop(L);
	lua_pushliteral(L, "uniforms");
	if (lua_compare(L, 2, -1, LUA_OPEQ)) {
		lua_pushcfunction(L, l_shaderProgramUniforms);
		return 1;
	}
	lua_pushliteral(L, "getAttributes");
	if (lua_compare(L, 2, -1, LUA_OPEQ)) {
		lua_settop(L, 1);
		lua_pushcclosure(L, lua_shaderProgramGetAttributes, 1);
		return 1;
	}
	//TODO: Add other methods here?
	// getProgramVertexArray
	// Maybe I want these implemented in Lua, not C

	// lua_pushliteral(L, "getProgramAttributes");
	// if (lua_compare(L, 2, -1, LUA_OPEQ)) {
	// 	lua_pushcfunction(L, lua_getProgramAttributes);
	// 	return 1;
	// }
	// lua_pushliteral(L, "getProgramVertexArray");
	// if (lua_compare(L, 2, -1, LUA_OPEQ)) {
	// 	lua_pushcfunction(L, lua_getProgramVertexArray);
	// 	return 1;
	// }

	//Get the uniforms table, copy the index to the top of the stack, index the table using it.
	lua_getiuservalue(L, 1, 1);
	lua_pushvalue(L, 2);
	lua_gettable(L, -2);
	//"value" field holds the last assigned value for the uniform (or nil)
	lua_pushliteral(L, "value");
	lua_gettable(L, -2);
	//For now, we'll just return the last value we set the uniform to through the Lua wrapper.
	//Later, could make it actually fetch the current value from OpenGL, for validation.
	return 1;
}

int l_shaderProgram__newindex(lua_State *L)
{
	//Only used to assign specific uniform by name
	lua_settop(L, 3);
	GLuint *shader = luaL_checkudata(L, 1, "tu.ShaderProgram");
	lua_getfield(L, LUA_REGISTRYINDEX, "tu_activeShaderProgram");
	if (!lua_compare(L, 1, -1, LUA_OPEQ))
		return luaL_error(L, "Shader program must be current active program to modify uniform variables.");
	const char *uniform_name = luaL_checkstring(L, 2);
	lua_getiuservalue(L, 1, 1);
	lua_pushvalue(L, 2);
	//Check type, if it's not a table, throw an error
	if (lua_gettable(L, 5) != LUA_TTABLE)
		return luaL_error(L, "Shader does not have a uniform with the name \"%s\".\n"
			"The uniform could have been optimized out if it was not referenced in the shader.", uniform_name);

	/* Stack now looks like this:
	1 | tu.ShaderProgram
	2 | index (k)
	3 | value (v)
	4 | active shader program
	5 | shader uniforms table
	6 | {type = <uniform_type>, count = <count>, [value = <value>]} (type table for the given uniform)
	*/

	//TODO: Defer sending the uniform until the shader is used to draw (so I don't need the shader bound)
	send_lua_value_as_uniform(L, 3, 6);
	CHECK_ERRORS()
	return 0;
}

static luaL_Reg l_opengl[] = {
	{"ActiveTexture", l_glActiveTexture},
	{"BindBuffer", l_glBindBuffer},
	{"BindTexture", l_glBindTexture},
	{"BindVertexArray", l_glBindVertexArray},
	{"Pack32f", l_glPack32f},
	{"Pack32i", l_glPack32i},
	{"BufferData", l_glBufferData},
	{"Clear", l_glClear},
	{"ClearColor", l_glClearColor},
	{"ClearDepth", l_glClearDepth},
	{"DeleteBuffers", l_Buffers__gc},
	{"DepthFunc", l_glDepthFunc},
	{"Disable", l_glDisable},
	{"DrawArraysInstanced", l_glDrawArraysInstanced},
	{"DrawElementsInstanced", l_glDrawElementsInstanced},
	{"DrawElementsInstancedBaseVertex", l_glDrawElementsInstancedBaseVertex},
	{"Enable", l_glEnable},
	{"EnableVertexAttribArray", l_glEnableVertexAttribArray},
	{"Buffers", l_Buffers},
	{"VertexArrays", l_vertexArrays},
	{"GetAttribLocation", l_glGetAttribLocation},
	{"GetUniformLocation", l_glGetUniformLocation},
	{"PolygonMode", l_glPolygonMode},
	{"PrimitiveRestartIndex", l_glPrimitiveRestartIndex},
	{"UseProgram", l_glUseProgram},
	{"VertexAttribDivisor", l_glVertexAttribDivisor},
	{"VertexAttribPointer", l_glVertexAttribPointer},
	{"VertexAttribIPointer", l_glVertexAttribIPointer},
	{"Viewport", l_glViewport},
	{"GetError", l_glGetError},
	{"VertexShader", lua_vertexShader},
	{"FragmentShader", lua_fragmentShader},
	{"GeometryShader", lua_geometryShader},
	{"ShaderProgram", lua_shaderProgram},
	//TODO(Gavin): Use placeholders here (NULL pointer) to simplify lib creation
	{NULL, NULL}
};

//Create a table of valid values for "glEnable"
//Example use: glEnable(GL_BLEND) in Lua is gl.Enable(gl.BLEND)
#define ENABLEVAL(name) {GL_##name, #name}
static struct {
	GLenum value;
	const char *name;
} l_glEnableVals[] = {
	ENABLEVAL(BLEND),
	ENABLEVAL(COLOR_LOGIC_OP),
	ENABLEVAL(CULL_FACE),
	// ENABLEVAL(DEBUG_OUTPUT),
	// ENABLEVAL(DEBUG_OUTPUT_SYNCHRONOUS),
	ENABLEVAL(DEPTH_CLAMP),
	ENABLEVAL(DEPTH_TEST),
	ENABLEVAL(DITHER),
	ENABLEVAL(FRAMEBUFFER_SRGB),
	ENABLEVAL(LINE_SMOOTH),
	ENABLEVAL(MULTISAMPLE),
	ENABLEVAL(POLYGON_OFFSET_FILL),
	ENABLEVAL(POLYGON_OFFSET_LINE),
	ENABLEVAL(POLYGON_OFFSET_POINT),
	ENABLEVAL(POLYGON_SMOOTH),
	ENABLEVAL(PRIMITIVE_RESTART),
	// ENABLEVAL(PRIMITIVE_RESTART_FIXED_INDEX),
	ENABLEVAL(RASTERIZER_DISCARD),
	ENABLEVAL(SAMPLE_ALPHA_TO_COVERAGE),
	ENABLEVAL(SAMPLE_ALPHA_TO_ONE),
	ENABLEVAL(SAMPLE_COVERAGE),
	ENABLEVAL(SAMPLE_SHADING),
	ENABLEVAL(SAMPLE_MASK),
	ENABLEVAL(SCISSOR_TEST),
	ENABLEVAL(STENCIL_TEST),
	ENABLEVAL(TEXTURE_CUBE_MAP_SEAMLESS),
	ENABLEVAL(PROGRAM_POINT_SIZE),
};

//Create a table of valid targets for "glBindBuffer"
#define BUFFERTARGET(name) {GL_##name, #name}
static struct {
	GLenum target;
	const char *name;
} l_glBindBufferTargets[] = {
	BUFFERTARGET(ARRAY_BUFFER),              //Vertex attributes
	BUFFERTARGET(ATOMIC_COUNTER_BUFFER),     //Atomic counter storage
	BUFFERTARGET(COPY_READ_BUFFER),          //Buffer copy source
	BUFFERTARGET(COPY_WRITE_BUFFER),         //Buffer copy destination
	// BUFFERTARGET(DISPATCH_INDIRECT_BUFFER),  //Indirect compute dispatch commands
	BUFFERTARGET(DRAW_INDIRECT_BUFFER),      //Indirect command arguments
	BUFFERTARGET(ELEMENT_ARRAY_BUFFER),      //Vertex array indices
	BUFFERTARGET(PIXEL_PACK_BUFFER),         //Pixel read target
	BUFFERTARGET(PIXEL_UNPACK_BUFFER),       //Texture data source
	// BUFFERTARGET(QUERY_BUFFER),              //Query result buffer
	// BUFFERTARGET(SHADER_STORAGE_BUFFER),     //Read-write storage for shaders
	BUFFERTARGET(TEXTURE_BUFFER),            //Texture data buffer
	BUFFERTARGET(TRANSFORM_FEEDBACK_BUFFER), //Transform feedback buffer
	BUFFERTARGET(UNIFORM_BUFFER),            //Uniform block storage
};

/*
STREAM
The data store contents will be modified once and used at most a few times.
STATIC
The data store contents will be modified once and used many times.
DYNAMIC
The data store contents will be modified repeatedly and used many times.

The nature of access may be one of these:

DRAW
The data store contents are modified by the application, and used as the source for GL drawing and image specification commands.
READ
The data store contents are modified by reading data from the GL, and used to return that data when queried by the application.
COPY
The data store contents are modified by reading data from the GL, and used as the source for GL drawing and image specification commands.
*/
//Create a table of valid usages for "glBindBuffer"
#define BUFFERUSAGE(name) {GL_##name, #name}
static struct {
	GLenum usage;
	const char *name;
} l_glBindBufferUsages[] = {
	BUFFERUSAGE(STREAM_DRAW),
	BUFFERUSAGE(STREAM_READ),
	BUFFERUSAGE(STREAM_COPY),
	BUFFERUSAGE(STATIC_DRAW),
	BUFFERUSAGE(STATIC_READ),
	BUFFERUSAGE(STATIC_COPY),
	BUFFERUSAGE(DYNAMIC_DRAW),
	BUFFERUSAGE(DYNAMIC_READ),
	BUFFERUSAGE(DYNAMIC_COPY),
};

#define CLEARBIT(name) {GL_##name, #name}
static struct {
	GLbitfield bit;
	const char *name;
} l_glClearBits[] = {
	CLEARBIT(COLOR_BUFFER_BIT),
	CLEARBIT(DEPTH_BUFFER_BIT),
	// CLEARBIT(ACCUM_BUFFER_BIT),
	CLEARBIT(STENCIL_BUFFER_BIT),
};

#define COLORBUFFER(name) {GL_##name, #name}
static struct {
	GLenum colorbuffer;
	const char *name;
} l_glColorBuffers[] = {
	COLORBUFFER(NONE),
	COLORBUFFER(FRONT_LEFT),
	COLORBUFFER(FRONT_RIGHT),
	COLORBUFFER(BACK_LEFT),
	COLORBUFFER(BACK_RIGHT),
	COLORBUFFER(FRONT),
	COLORBUFFER(BACK),
	COLORBUFFER(LEFT),
	COLORBUFFER(RIGHT),
	COLORBUFFER(FRONT_AND_BACK),
};

#define DEPTHFUNC(name) {GL_##name, #name}
static struct {
	GLenum func;
	const char *name;
} l_glDepthFuncs[] = {
	DEPTHFUNC(NEVER),
	DEPTHFUNC(LESS),
	DEPTHFUNC(EQUAL),
	DEPTHFUNC(LEQUAL),
	DEPTHFUNC(GREATER),
	DEPTHFUNC(NOTEQUAL),
	DEPTHFUNC(GEQUAL),
	DEPTHFUNC(ALWAYS),
};

#define POLYGONMODE(name) {GL_##name, #name}
static struct {
	GLenum mode;
	const char *name;
} l_glPolygonModes[] = {
	POLYGONMODE(POINT),
	POLYGONMODE(LINE),
	POLYGONMODE(FILL),
};

#define PRIMITIVEMODE(name) {GL_##name, #name}
static struct {
	GLenum mode;
	const char *name;
} l_glPrimitiveModes[] = {
	PRIMITIVEMODE(POINTS),
	PRIMITIVEMODE(LINE_STRIP),
	PRIMITIVEMODE(LINE_LOOP),
	PRIMITIVEMODE(LINES),
	PRIMITIVEMODE(LINE_STRIP_ADJACENCY),
	PRIMITIVEMODE(LINES_ADJACENCY),
	PRIMITIVEMODE(TRIANGLE_STRIP),
	PRIMITIVEMODE(TRIANGLE_FAN),
	PRIMITIVEMODE(TRIANGLES),
	PRIMITIVEMODE(TRIANGLE_STRIP_ADJACENCY),
	PRIMITIVEMODE(TRIANGLES_ADJACENCY),
	PRIMITIVEMODE(PATCHES),
};

#define FRAMEBUFFERTARGET(name) {GL_##name, #name}
static struct {
	GLenum target;
	const char *name;
} l_glFramebufferTargets[] = {
	FRAMEBUFFERTARGET(FRAMEBUFFER),
	FRAMEBUFFERTARGET(DRAW_FRAMEBUFFER),
	FRAMEBUFFERTARGET(READ_FRAMEBUFFER),
};

#define POINTERTYPE(name) {GL_##name, #name}
static struct {
	GLenum type;
	const char *name;
} l_glPointerTypes[] = {
	POINTERTYPE(BYTE),
	POINTERTYPE(UNSIGNED_BYTE),
	POINTERTYPE(SHORT),
	POINTERTYPE(UNSIGNED_SHORT),
	POINTERTYPE(INT),
	POINTERTYPE(UNSIGNED_INT),
	POINTERTYPE(HALF_FLOAT),
	POINTERTYPE(FLOAT),
	POINTERTYPE(DOUBLE),
	POINTERTYPE(FIXED),
	POINTERTYPE(INT_2_10_10_10_REV),
	POINTERTYPE(UNSIGNED_INT_2_10_10_10_REV),
	POINTERTYPE(UNSIGNED_INT_10F_11F_11F_REV),
	POINTERTYPE(DOUBLE),
};

int luaopen_l_opengl(lua_State *L)
{
	//Set up garbage-collection functions for Shader and ShaderProgram userdata
	luaL_newmetatable(L, "tu.Shader");
	lua_pushcfunction(L, l_glDeleteShader);
	lua_setfield(L, -2, "__gc");

	luaL_newmetatable(L, "tu.ShaderProgram");
	lua_pushcfunction(L, l_glDeleteProgram);
	lua_setfield(L, -2, "__gc");
	lua_pushcfunction(L, lua_shaderProgram__index);
	lua_setfield(L, -2, "__index");
	lua_pushcfunction(L, l_shaderProgram__newindex);
	lua_setfield(L, -2, "__newindex");
	lua_pushcfunction(L, l_shaderProgramUniforms);
	lua_setfield(L, -2, "uniforms");
	#define STRINGIFY(x) #x
	#define VAL_AS_STR(x) STRINGIFY(x)
	
	//Source for any builtin functions, which will be compiled once then assigned into the registry
	lua_pushliteral(L,
		"local VERTEX_SHADER_TYPE = "VAL_AS_STR(GL_VERTEX_SHADER)"\n"
		"local SHADER_TYPES = {float = "VAL_AS_STR(TULUA_TFLOAT)", int = "VAL_AS_STR(TULUA_TINT)", bool = "VAL_AS_STR(TULUA_TBOOL)", vec2 = "VAL_AS_STR(TULUA_TVEC2)", vec3 = "VAL_AS_STR(TULUA_TVEC3)", vec4 = "VAL_AS_STR(TULUA_TVEC4)", mat3 = "VAL_AS_STR(TULUA_TMAT3)", mat4 = "VAL_AS_STR(TULUA_TMAT4)", sampler2D = "VAL_AS_STR(TULUA_TTEX)"}\n");
	lua_pushstring(L, (const char []){
		#embed "builtin_opengl_fns.lua" suffix(, 0)
	});
	lua_concat(L, 2);
	luaL_loadstring(L, lua_tostring(L, -1));
	lua_call(L, 0, LUA_MULTRET);

	//We're popping these off the stack, so reverse the order from above!
	lua_setfield(L, LUA_REGISTRYINDEX, "tu_builtin_shaderSourceError");
	lua_setfield(L, LUA_REGISTRYINDEX, "tu_builtin_uniformsFromShaderString");
	lua_setfield(L, LUA_REGISTRYINDEX, "tu_builtin_attributesFromShaderStrings");
	lua_createtable(L, 80, 0);
	lua_setfield(L, LUA_REGISTRYINDEX, "tu_gl_texture_units");

	luaL_newmetatable(L, "tu.gl.VertexArrays");
	lua_pushcfunction(L, l_VertexArrays__gc);
	lua_setfield(L, -2, "__gc");
	lua_pushcfunction(L, l_VertexArrays__index);
	lua_setfield(L, -2, "__index");

	luaL_newmetatable(L, "tu.gl.VertexArray");
	lua_pushcfunction(L, l_VertexArray__gc);
	lua_setfield(L, -2, "__gc");
	lua_pushcfunction(L, l_VertexArray__tostring);
	lua_setfield(L, -2, "__tostring");

	luaL_newmetatable(L, "tu.gl.Buffers");
	lua_pushcfunction(L, l_Buffers__gc);
	lua_setfield(L, -2, "__gc");
	lua_pushcfunction(L, l_Buffers__index);
	lua_setfield(L, -2, "__index");
	lua_pushcfunction(L, l_Buffers__len);
	lua_setfield(L, -2, "__len");

	luaL_newmetatable(L, "tu.gl.Buffer");
	lua_pushcfunction(L, l_Buffer__gc);
	lua_setfield(L, -2, "__gc");
	lua_pushvalue(L, -2); //Copy the tu.gl.Buffer metatable so we can set it as its __index 
	lua_setfield(L, -2, "__index");

	luaL_newmetatable(L, "tu.gl.Framebuffers");
	lua_pushcfunction(L, l_Framebuffers__gc);
	lua_setfield(L, -2, "__gc");
	lua_pushcfunction(L, l_Framebuffers__index);
	lua_setfield(L, -2, "__index");
	lua_pushcfunction(L, l_Framebuffers__len);
	lua_setfield(L, -2, "__len");

	luaL_newmetatable(L, "tu.gl.Framebuffer");
	lua_pushcfunction(L, l_Framebuffer__gc);
	lua_setfield(L, -2, "__gc");
	lua_pushvalue(L, -2); //Copy the tu.gl.Framebuffer metatable so we can set it as its __index 
	lua_setfield(L, -2, "__index");

	luaL_newmetatable(L, "tu.gl.EnableValue");
	luaL_newmetatable(L, "tu.gl.BufferTarget");
	luaL_newmetatable(L, "tu.gl.BufferUsage");
	luaL_newmetatable(L, "tu.gl.ColorBuffer");
	luaL_newmetatable(L, "tu.gl.DepthFunc");
	luaL_newmetatable(L, "tu.gl.PolygonMode");
	luaL_newmetatable(L, "tu.gl.PrimitiveMode");
	luaL_newmetatable(L, "tu.gl.PointerType");
	luaL_newmetatable(L, "tu.gl.PackedBuffer");
	luaL_newmetatable(L, "tu.gl.FramebufferTarget");

#define LENGTH(arr) (sizeof((arr))/sizeof(((arr)[0])))

	//Create a table with enough room for all funcs and glEnable defines
	//+1 for uniformsFromShaderString
	size_t nrec = LENGTH(l_opengl) + 1 +
		+ LENGTH(l_glEnableVals)
		+ LENGTH(l_glBindBufferTargets)
		+ LENGTH(l_glBindBufferUsages)
		+ LENGTH(l_glPolygonModes)
		+ LENGTH(l_glPrimitiveModes)
		+ LENGTH(l_glFramebufferTargets)
		+ LENGTH(l_glPointerTypes)
		+ LENGTH(l_glColorBuffers)
		+ LENGTH(l_glDepthFuncs)
		+ LENGTH(l_glClearBits);

	lua_createtable(L, 0, nrec);
	//Assign the functions into the table
	luaL_setfuncs(L, l_opengl, 0);

#define SET_USERDATA_ENUMS(udname,sarray,sarraylen,sfieldname) \
	for (int i = 0; i < sarraylen; i++) { \
		GLenum *udenum = lua_newuserdatauv(L, sizeof(GLenum), 0); \
		*udenum = sarray[i].sfieldname; \
		luaL_setmetatable(L, udname); \
		lua_setfield(L, -2, sarray[i].name); \
	}

	SET_USERDATA_ENUMS("tu.gl.EnableValue", l_glEnableVals, LENGTH(l_glEnableVals), value)
	SET_USERDATA_ENUMS("tu.gl.BufferTarget", l_glBindBufferTargets, LENGTH(l_glBindBufferTargets), target)
	SET_USERDATA_ENUMS("tu.gl.BufferUsage", l_glBindBufferUsages, LENGTH(l_glBindBufferUsages), usage)
	SET_USERDATA_ENUMS("tu.gl.ColorBuffer", l_glColorBuffers, LENGTH(l_glColorBuffers), colorbuffer)
	SET_USERDATA_ENUMS("tu.gl.DepthFunc", l_glDepthFuncs, LENGTH(l_glDepthFuncs), func)
	SET_USERDATA_ENUMS("tu.gl.PolygonMode", l_glPolygonModes, LENGTH(l_glPolygonModes), mode)
	SET_USERDATA_ENUMS("tu.gl.PrimitiveMode", l_glPrimitiveModes, LENGTH(l_glPrimitiveModes), mode)
	SET_USERDATA_ENUMS("tu.gl.FramebufferTarget", l_glFramebufferTargets, LENGTH(l_glFramebufferTargets), target)
	SET_USERDATA_ENUMS("tu.gl.PointerType", l_glPointerTypes, LENGTH(l_glPointerTypes), type)

	//Assign the glClear bits into the table
	for (int i = 0; i< LENGTH(l_glClearBits); i++) {
		lua_pushinteger(L, l_glClearBits[i].bit);
		lua_setfield(L, -2, l_glClearBits[i].name);
	}

	return 1;
}