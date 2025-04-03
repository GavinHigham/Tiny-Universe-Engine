//Lua headers
#include <lua-5.4.4/src/lua.h>
#include <lua-5.4.4/src/lauxlib.h>
#include <lua-5.4.4/src/lualib.h>
#include "graphics.h"
#include "shader_utils.h"

/*
TODO: 8/12/2024
Need to add "new" function for texture, set __index metamethods so methods can be called
Maybe use "closure" style for functions that do not modify the left-hand?
So myimage.toTexture() rather than myimage:toTexture()

Or maybe I just do whichever is easiest.

TODO: 8/19/2024
Need to add all the numeric constants for texture parameters, possibly wrapped with userdata type?
----------------------------------------------------------------------------------------------

In the simplest case, I want to be able to load an image from a file with just the filepath
string, and get an object with sensible defaults that will be cleaned up if made nil.

local myimage = image.fromFile('myimage.png')
local mytexture = myimage:toTexture(gl.TEXTURE_2D, {MIN_FILTER = gl.NEAREST, MAG_FILTER = gl.NEAREST})

--or
local mytexture2 = myimage:toTexture(gl.TEXTURE_2D)
mytexture2:setParameters{MIN_FILTER = gl.NEAREST, MAG_FILTER = gl.NEAREST}

Should be able to change details with other APIs (essentially wrapping what SDL and OpenGL provide)
Texture object should inherit appropriate details from image, such as texture format and resolution.

I'll take this function and split it into two. The first will load the image and save it as a userdata
object, the second will take that userdata object and create an OpenGL texture, with a different userdata.

I'll get rid of the checks for number of texture channels, and just use the correct format in the OpenGL call.

GLuint load_gl_texture(char *path)
{
	GLuint texture = 0;
	SDL_Surface *surface = IMG_Load(path);
	GLenum texture_format;
	if (!surface) {
		printf("Texture %s could not be loaded.\n", path);
		return 0;
	}

	// Get the number of channels in the SDL surface.
	int num_colors = surface->format->BytesPerPixel;
	bool rgb = surface->format->Rmask == 0x000000ff;
	if (num_colors == 4) {
		texture_format = rgb ? GL_RGBA : GL_BGRA;
	} else if (num_colors == 3) {
		texture_format = rgb ? GL_RGB : GL_BGR;
	} else {
		printf("Image does not have at least 3 color channels.\n");
		goto error;
	}

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	SDL_LockSurface(surface);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, surface->w, surface->h, 0, texture_format, GL_UNSIGNED_BYTE, surface->pixels);
	SDL_UnlockSurface(surface);

error:
	SDL_FreeSurface(surface);
	return texture;
}
*/

static int l_image_Surface(lua_State *L)
{
	SDL_Surface *surface = SDL_CreateSurface(
		luaL_checkinteger(L, 1),
		luaL_checkinteger(L, 2),
		SDL_GetPixelFormatForMasks(
			luaL_optinteger(L, 3, 32),
			luaL_optinteger(L, 4, 0),
			luaL_optinteger(L, 5, 0),
			luaL_optinteger(L, 6, 0),
			luaL_optinteger(L, 7, 0)));
	if (!surface)
		return luaL_error(L, "Could not create a Surface");
	SDL_Surface **surface_ud = lua_newuserdatauv(L, sizeof(SDL_Surface *), 0);
	*surface_ud = surface;
	luaL_setmetatable(L, "tu.image.surface");
	return 1;
}

static int l_image_Surface_fromFile(lua_State *L)
{
	const char *path = luaL_checkstring(L, 1);
	SDL_Surface **surface = lua_newuserdatauv(L, sizeof(SDL_Surface *), 0);
	*surface = IMG_Load(path);
	if (!*surface)
		return luaL_error(L, "Could not load image at path: %s", path);
	luaL_setmetatable(L, "tu.image.surface");
	return 1;
}

static int l_image__gc(lua_State *L)
{
	SDL_Surface **surface = luaL_checkudata(L, 1, "tu.image.surface");
	SDL_DestroySurface(*surface);
	return 0;
}

//The texture should be bound already to the correct target using glBindTexture
static void l_image_texture_setParameter(lua_State *L, GLenum target, int key, int val)
{
	const char *pnamestrs[] = {
		//These are special cases
		[0] = "invalid",
		[1] = "target",
		[2] = "format",
		[3] = "internal_format",
		//These 3 are float values, call with glTexParameterf
		[4] = "TEXTURE_LOD_BIAS",
		[5] = "TEXTURE_MIN_LOD",
		[6] = "TEXTURE_MAX_LOD",
		//These two are vector, call with glTexParameteriv (or glTexParameterfv for BORDER_COLOR)
		[7] = "border_color",
		[8] = "TEXTURE_SWIZZLE_RGBA",
		//The rest are integer, call with glTexParameteri
		"DEPTH_STENCIL_TEXTURE_MODE",
		"TEXTURE_BASE_LEVEL",
		"TEXTURE_COMPARE_FUNC",
		"TEXTURE_COMPARE_MODE",
		"min_filter",
		"mag_filter",
		"TEXTURE_MAX_LEVEL",
		"TEXTURE_SWIZZLE_R",
		"TEXTURE_SWIZZLE_G",
		"TEXTURE_SWIZZLE_B",
		"TEXTURE_SWIZZLE_A",
		"wrap_s",
		"wrap_t",
		"wrap_r",
		NULL,
	};

	const GLenum pnames[] = {
		//These are special cases
		[0] = 0,
		[1] = 1,
		[2] = 2,
		[3] = 3,
		//These first 3 are float values, call with glTexParameterf
		[4] = GL_TEXTURE_LOD_BIAS,
		[5] = GL_TEXTURE_MIN_LOD,
		[6] = GL_TEXTURE_MAX_LOD,
		//These two are vector, call with glTexParameteriv (or glTexParameterfv for BORDER_COLOR)
		[7] = GL_TEXTURE_BORDER_COLOR,
		[8] = GL_TEXTURE_SWIZZLE_RGBA,
		//The rest are integer, call with glTexParameteri
		0,// GL_DEPTH_STENCIL_TEXTURE_MODE,
		GL_TEXTURE_BASE_LEVEL,
		GL_TEXTURE_COMPARE_FUNC,
		GL_TEXTURE_COMPARE_MODE,
		GL_TEXTURE_MIN_FILTER,
		GL_TEXTURE_MAG_FILTER,
		GL_TEXTURE_MAX_LEVEL,
		GL_TEXTURE_SWIZZLE_R,
		GL_TEXTURE_SWIZZLE_G,
		GL_TEXTURE_SWIZZLE_B,
		GL_TEXTURE_SWIZZLE_A,
		GL_TEXTURE_WRAP_S,
		GL_TEXTURE_WRAP_T,
		GL_TEXTURE_WRAP_R,
		0,
	};

	//Index in the string / enum arrays above
	int idx = luaL_checkoption(L, key, "invalid", pnamestrs);
	GLenum pname = pnames[idx];
	//If it's a vector param, we use one of these
	GLint i4[4] = {0,0,0,0};
	GLfloat f4[4] = {0.0,0.0,0.0,0.0};

	switch (idx) {
	case 0:
		luaL_error(L, "Invalid parameter name %s", luaL_checkstring(L, key));
	case 1:
	case 2:
	case 3:
		//These three are ignored, we already pulled them out in l_image_texture_setParameters_internal
		break;
	case 4:
	case 5:
	case 6:
		glTexParameterf(target, pname, luaL_checknumber(L, val));
		return;
	case 7:
	case 8:
		if (!lua_istable(L, val))
			luaL_error(L, "Expected table value for parameter %s", pnamestrs[idx]);

		int top = lua_gettop(L);
		lua_geti(L, val, 1);
		lua_geti(L, val, 2);
		lua_geti(L, val, 3);
		lua_geti(L, val, 4);
		//border_color can be float or integer, check if integers were used
		if (idx == 7 && !lua_isinteger(L, top+1)) {
			f4[0] = luaL_checknumber(L, top+1);
			f4[1] = luaL_checknumber(L, top+2);
			f4[2] = luaL_checknumber(L, top+3);
			f4[3] = luaL_checknumber(L, top+4);
			glTexParameterfv(target, pname, f4);
			lua_pop(L, 4);
			return;
		}
		i4[0] = luaL_checkinteger(L, top+1);
		i4[1] = luaL_checkinteger(L, top+2);
		i4[2] = luaL_checkinteger(L, top+3);
		i4[3] = luaL_checkinteger(L, top+4);
		glTexParameteriv(target, pname, i4);
		lua_pop(L, 4);
		return;
	default:
		glTexParameteri(target, pname, luaL_checkinteger(L, val));
		return;
	}
}

static GLenum l_image_texture_target_stoi(lua_State *L, int target_i)
{
	//TODO: Convert the string into its OpenGL enum value (should make a Lua table for this)
	return GL_TEXTURE_2D;
}

static GLenum l_image_texture_format_stoi(lua_State *L, int internal_format_i)
{
	const char* internal_format_strs[] = {
		"DEPTH_COMPONENT",
		"DEPTH_STENCIL",
		"RED",
		"RG",
		"RGB",
		"RGBA",
		"RGBA8",
		"RGBA16",
	};

	const GLenum internal_formats[] = {
		GL_DEPTH_COMPONENT,
		GL_DEPTH_STENCIL,
		GL_RED,
		GL_RG,
		GL_RGB,
		GL_RGBA,
		GL_RGBA8,
		GL_RGBA16,
	};

	int format_i = luaL_checkoption(L, internal_format_i, "RGBA", internal_format_strs);

	return internal_formats[format_i];
	//TODO: do this with a Lua table instead
}

static void l_image_texture_setParameters_internal(lua_State *L, int t, GLuint texture, GLenum *target, GLenum *format, GLenum *internal_format)
{
	GLenum target_final = GL_TEXTURE_2D;
	luaL_checktype(L, t, LUA_TTABLE);

	//Check if the parameters table defines a "target", "format", and "internal_format"
	if (target) {
		lua_pushliteral(L, "target");
		if (lua_gettable(L, t) == LUA_TSTRING)
			target_final = l_image_texture_target_stoi(L, lua_absindex(L, -1));
		*target = target_final;
		lua_pop(L, 1);
	}

	if (format) {
		lua_pushliteral(L, "format");
		if (lua_gettable(L, t) == LUA_TSTRING)
			*format = l_image_texture_format_stoi(L, lua_absindex(L, -1));
		else
			*format = GL_RGBA;
		lua_pop(L, 1);
	}

	if (internal_format) {
		lua_pushliteral(L, "internal_format");
		if (lua_gettable(L, t) == LUA_TSTRING)
			*internal_format = l_image_texture_format_stoi(L, lua_absindex(L, -1));
		else if (format)
			*internal_format = *format;
		else
			*internal_format = GL_RGBA;
		lua_pop(L, 1);
	}

	//Set defaults for min/mag filter and texture wrap
	//We need these to get a complete texture if these are not defined in the table
	glBindTexture(target_final, texture);
	lua_pushliteral(L, "min_filter");
	if (lua_gettable(L, t) == LUA_TNIL) {
		glTexParameteri(target_final, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	}
	lua_pushliteral(L, "mag_filter");
	if (lua_gettable(L, t) == LUA_TNIL) {
		glTexParameteri(target_final, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}
	lua_pushliteral(L, "wrap_s");
	if (lua_gettable(L, t) == LUA_TNIL) {
		glTexParameteri(target_final, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	}
	lua_pushliteral(L, "wrap_t");
	if (lua_gettable(L, t) == LUA_TNIL) {
		glTexParameteri(target_final, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
	lua_pop(L, 4);

	//Iterate over the parameter table, setting each one
	lua_pushnil(L);
	while (lua_next(L, t) != 0) {
		l_image_texture_setParameter(L, target_final, lua_absindex(L, -2), lua_absindex(L, -1));
		lua_pop(L, 1);
	}
}

//NOTE: Possibly unexpected results if the target is set to a different value after the texture has been created.
static int l_image_texture_setParameters(lua_State *L)
{
	GLuint *texture = luaL_checkudata(L, 1, "tu.image.texture");
	//Retrieve the target from the first uservalue of the texture userdata
	lua_getiuservalue(L, 1, 1);
	GLenum target = lua_tointeger(L, -1);
	l_image_texture_setParameters_internal(L, 1, *texture, &target, NULL, NULL);
	return 0;
}

static int l_image_texture_getSize(lua_State *L)
{
	GLuint *texture = luaL_checkudata(L, 1, "tu.image.texture");
	//Retrieve the target from the first uservalue of the texture userdata
	lua_getiuservalue(L, 1, 1);
	GLenum target = lua_tointeger(L, -1);
	GLint width, height;
	glBindTexture(target, *texture);
	glGetTexLevelParameteriv(target, 0, GL_TEXTURE_WIDTH, &width);
	glGetTexLevelParameteriv(target, 0, GL_TEXTURE_HEIGHT, &height);
	lua_pushinteger(L, width);
	lua_pushinteger(L, height);
	return 2;
}

static int l_image_texture_active(lua_State *L)
{
	//TODO: Replace this extremely dumb, brittle code.
	static int unused_texture_unit = 0;
	int texture_unit = luaL_optinteger(L, 2, unused_texture_unit);
	//Pick the next texture unit, wrapping around after 80.
	//Relying on these being stable over time is unwise.
	unused_texture_unit = (unused_texture_unit + 1) % 80;

	GLuint *texture = luaL_checkudata(L, 1, "tu.image.texture");
	//Retrieve the target from the first uservalue of the texture userdata
	lua_getiuservalue(L, 1, 1);
	GLenum target = lua_tointeger(L, -1);

	glActiveTexture(GL_TEXTURE0 + texture_unit);
	glBindTexture(target, *texture);
	lua_pushinteger(L, texture_unit);
	return 1;
}

static void l_checkframebufferstatus(lua_State *L, GLenum fbo_binding, GLint fbo)
{
//GL_FRAMEBUFFER_UNDEFINED is returned if the specified framebuffer is the default read or draw framebuffer, but the default framebuffer does not exist.
//GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT is returned if any of the framebuffer attachment points are framebuffer incomplete.
//GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT is returned if the framebuffer does not have at least one image attached to it.
//GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER is returned if the value of GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE is GL_NONE for any color attachment point(s) named by GL_DRAW_BUFFERi.
//GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER is returned if GL_READ_BUFFER is not GL_NONE and the value of GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE is GL_NONE for the color attachment point named by GL_READ_BUFFER.
//GL_FRAMEBUFFER_UNSUPPORTED is returned if the combination of internal formats of the attached images violates an implementation-dependent set of restrictions.
//GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE is returned if the value of GL_RENDERBUFFER_SAMPLES is not the same for all attached renderbuffers; if the value of GL_TEXTURE_SAMPLES is the not same for all attached textures; or, if the attached images are a mix of renderbuffers and textures, the value of GL_RENDERBUFFER_SAMPLES does not match the value of GL_TEXTURE_SAMPLES.
//GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE is also returned if the value of GL_TEXTURE_FIXED_SAMPLE_LOCATIONS is not the same for all attached textures; or, if the attached images are a mix of renderbuffers and textures, the value of GL_TEXTURE_FIXED_SAMPLE_LOCATIONS is not GL_TRUE for all attached textures.
//GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS is returned if any framebuffer attachment is layered, and any populated attachment is not layered, or if all populated color attachments are not from textures of the same target.
	glBindFramebuffer(fbo_binding, fbo);
	GLenum glerror = glCheckFramebufferStatus(fbo_binding);
	switch(glerror) {
	case GL_FRAMEBUFFER_UNDEFINED:
		luaL_error(L, "Error %d: Framebuffer undefined.", glerror); return;
	case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
		luaL_error(L, "Error %d: Framebuffer incomplete attachment.", glerror); return;
	case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
		luaL_error(L, "Error %d: Framebuffer incomplete missing attachment.", glerror); return;
	case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
		luaL_error(L, "Error %d: Framebuffer incomplete draw buffer.", glerror); return;
	case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
		luaL_error(L, "Error %d: Framebuffer incomplete read buffer.", glerror); return;
	case GL_FRAMEBUFFER_UNSUPPORTED:
		luaL_error(L, "Error %d: Framebuffer unsupported.", glerror); return;
	case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
		luaL_error(L, "Error %d: Framebuffer incomplete multisample.", glerror); return;
	case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
		luaL_error(L, "Error %d: Framebuffer incomplete layer targets.", glerror); return;
	}
}

static int l_image_texture_drawTo(lua_State *L)
{
	GLuint *texture = luaL_checkudata(L, 1, "tu.image.texture");
	luaL_checktype(L, 2, LUA_TFUNCTION);

	//Retrieve the target from the first uservalue of the texture userdata
	if (lua_getiuservalue(L, 1, 1) != LUA_TNUMBER)
		return luaL_error(L, "Texture does not have a target set.");
	GLenum target = lua_tointeger(L, -1);

	//Retrieve the format from the second uservalue of the texture userdata
	lua_getiuservalue(L, 1, 2);
	GLenum format = lua_tointeger(L, -1);

	//Get the texture dimensions from OpenGL
	GLint width, height;
	glBindTexture(target, *texture);
	glGetTexLevelParameteriv(target, 0, GL_TEXTURE_WIDTH, &width);
	glGetTexLevelParameteriv(target, 0, GL_TEXTURE_HEIGHT, &height);

	//Check if the texture already has a framebuffer stored in uservalue 3
	GLuint fbo;
	if (lua_getiuservalue(L, 1, 3) == LUA_TNUMBER) {
		fbo = lua_tointeger(L, -1);
	} else {
		//tex:drawTo(drawfn, true) skips creating depth attachment
		//TODO: non-bool arg 3 specifies the generated depth texture format.
		//TODO: maybe also use renderbuffer for depth as an optimization
		bool skipdepth = false;
		if (lua_isboolean(L, 3))
			skipdepth = lua_toboolean(L, 3);

		//Create and store the fbo
		lua_pop(L, 1);
		glGenFramebuffers(1, &fbo);
		lua_pushinteger(L, fbo);
		lua_setiuservalue(L, 1, 3);

		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
		glBindTexture(target, *texture);

		//If we're a color texture and skipdepth is false, create and attach a depth texture
		switch(format) {
			//If we're a depth texture, just attach to the framebuffer, don't make a color buffer.
		case GL_DEPTH_COMPONENT:
		case GL_DEPTH_STENCIL:
			glDrawBuffer(GL_NONE);
			glReadBuffer(GL_NONE);
			glBindTexture(GL_TEXTURE_2D, *texture);
			glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, *texture, 0);
			break;
		default: //GL_RED, GL_RG, GL_RGB, GL_RGBA
			glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, *texture, 0);
			glDrawBuffer(GL_COLOR_ATTACHMENT0);
			if (!skipdepth) {
				GLuint depth;
				glGenTextures(1, &depth);
				glBindTexture(GL_TEXTURE_2D, depth);
				//TODO: Find a nice way for the user to opt for a depth-stencil texture here (see skipdepth def.)
				glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
				//Store the depth texture in uservalue 4
				lua_pushinteger(L, depth);
				lua_setiuservalue(L, 1, 4);

				glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth, 0);
			}
		}
		l_checkframebufferstatus(L, GL_DRAW_FRAMEBUFFER, fbo);
	}

	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);
	glViewport(0, 0, width, height);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
	//If we're a color texture and skipdepth is false, create and attach a depth texture
	lua_settop(L, 2); //Wipe out anything above the function argument, then call it.
	lua_call(L, 0, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
	return 0;
}

static int l_image_Texture(lua_State *L)
{
	lua_Integer width = luaL_optinteger(L, 1, 512);
	lua_Integer height = luaL_optinteger(L, 2, width);
	GLenum target = GL_TEXTURE_2D, format = GL_RGBA, internal_format = GL_RGBA;
	GLuint *texture = lua_newuserdatauv(L, sizeof(GLuint), 4);
	int texidx = lua_absindex(L, -1);
	glGenTextures(1, texture);
	luaL_setmetatable(L, "tu.image.texture");

	//The param table could be arg 3, if both width and height are provided.
	bool params_set = false;
	for (int i = 1; i <= 3; i++) {
		if (lua_istable(L, i)) {
			l_image_texture_setParameters_internal(L, i, *texture, &target, &format, &internal_format);
			params_set = true;
			break;
		}
	}
	glBindTexture(target, *texture);
	if (!params_set) {

		glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}

	//Set the target to the first uservalue of the texture userdata
	lua_pushinteger(L, target);
	lua_setiuservalue(L, texidx, 1);
	//Set the format to the second uservalue of the texture userdata
	lua_pushinteger(L, format);
	lua_setiuservalue(L, texidx, 2);

	glTexImage2D(target, 0, internal_format, width, height, 0, format, GL_UNSIGNED_BYTE, NULL);
	return 1;
}

static int l_image_toTexture(lua_State *L)
{
	SDL_Surface **psurface = luaL_checkudata(L, 1, "tu.image.surface");
	SDL_Surface *surface = *psurface;
	GLenum target = GL_TEXTURE_2D, format = GL_RGBA, internal_format = GL_RGBA;
	GLuint *texture = lua_newuserdatauv(L, sizeof(GLuint), 4);
	int texidx = lua_absindex(L, -1);

	glGenTextures(1, texture);
	luaL_setmetatable(L, "tu.image.texture");

	if (lua_istable(L, 2)) {
		l_image_texture_setParameters_internal(L, 2, *texture, &target, &format, &internal_format);
	} else {
		glBindTexture(target, *texture);
		glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}

	//Set the target to the first uservalue of the texture userdata
	lua_pushinteger(L, target);
	lua_setiuservalue(L, texidx, 1);

	//Choose a format based on the SDL surface format (ignore the one from l_image_texture_setParameters_internal)
	const SDL_PixelFormatDetails *details = SDL_GetPixelFormatDetails(surface->format);
	bool rgb = details->Rmask == 0x000000ff;
	switch(details->bytes_per_pixel) {
	case 1: //TODO
		return luaL_error(L, "Single-channel images are not yet supported.");
	case 3: //No alpha channel
		format = rgb ? GL_RGB : GL_BGR;
		break;
	case 4: //All 4 channels
		format = rgb ? GL_RGBA : GL_BGRA;
		break;
	default:
		return luaL_error(L, "Unsupported BytesPerPixel value in SDL_Surface.");
	}

	//Set the format to the second uservalue of the texture userdata
	lua_pushinteger(L, format);
	lua_setiuservalue(L, texidx, 2);

	SDL_LockSurface(surface);
	glBindTexture(target, *texture);
	glTexImage2D(target, 0, internal_format, surface->w, surface->h, 0, format, GL_UNSIGNED_BYTE, surface->pixels);
	SDL_UnlockSurface(surface);

	return 1;
}

static int l_image_texture__gc(lua_State *L)
{
	GLuint *texture = luaL_checkudata(L, 1, "tu.image.texture");
	//Check if we behind-the-scenes generated a framebuffer, and delete it
	if (lua_getiuservalue(L, 1, 3)) {
		GLuint fbo = lua_tointeger(L, -1);
		glDeleteFramebuffers(1, &fbo);
	}
	//Check if we behind-the-scenes generated a depth texture, and delete it
	if (lua_getiuservalue(L, 1, 4) == LUA_TNUMBER) {
		GLuint depth_texture = lua_tointeger(L, -1);
		glDeleteTextures(1, &depth_texture);
	}
	glDeleteTextures(1, texture);
	return 0;
}

static const luaL_Reg image_lib[] = {
	{"Surface", l_image_Surface},
	{"Texture", l_image_Texture},
	{"fromFile", l_image_Surface_fromFile},
	{NULL, NULL}
};

static const luaL_Reg image_surface_mt[] = {
	{"toTexture", l_image_toTexture},
	{"__gc", l_image__gc},
	{"__index", NULL},
	{NULL, NULL}
};

static const luaL_Reg image_texture_mt[] = {
	{"setParameters", l_image_texture_setParameters},
	{"getSize", l_image_texture_getSize},
	{"drawTo", l_image_texture_drawTo},
	{"active", l_image_texture_active},
	{"__gc", l_image_texture__gc},
	{"__index", NULL},
	{NULL, NULL}
};

int luaopen_l_image(lua_State *L)
{
	luaL_newmetatable(L, "tu.image.surface");
	luaL_setfuncs(L, image_surface_mt, 0);
	lua_setfield(L, -1, "__index"); //Indexing a surface indexes the metatable
	luaL_newmetatable(L, "tu.image.texture");
	luaL_setfuncs(L, image_texture_mt, 0);
	lua_setfield(L, -1, "__index"); //Indexing a texture indexes the metatable
	luaL_newlib(L, image_lib);

	return 1;
}