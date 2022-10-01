local gl = require 'OpenGL'
proctri = {}

--Hacky implementation of a tiny part of the GLSW library
local function glsw(str)
	local matches = {}
	local i,j,match,j_prev,match_prev = nil,nil
	repeat
		i,j,match = str:find('%s*%-%-%s*([%w%.]*).-\n', (j or 0)+1)
		if match_prev then
			matches[match_prev] = str:sub(j_prev+1, (i or 0)-1)
		end
		j_prev, match_prev = j, match
	until not match
	return matches
end

function proctri.init()
	vertexArrays = gl.VertexArrays(1)
	gl.BindVertexArray(vertexArrays[1])

	local shaders = glsw(io.open('shaders/glsw/atmosphere.glsl', 'r'):read('a'))
	local common = glsw(io.open('shaders/glsw/common.glsl', 'r'):read('a'))
	local vs = gl.VertexShader('#version 330\n' .. shaders['vertex.GL33'])
	local fs = gl.FragmentShader(table.concat({'#version 330',
		shaders['fragment.GL33'], common['utility'], common['noise'],
		common['lighting'], common['debugging']}, '\n'))
	local shaderProgram, log = gl.ShaderProgram(vs, fs)
	if log then
		print(log)
	end
	_G.shaderProgram = shaderProgram
	-- print("#shaderProgram = "..#shaderProgram)

	gl.Enable(gl.DEPTH_TEST);
	gl.Enable(gl.PRIMITIVE_RESTART);
	gl.Disable(gl.CULL_FACE);

	return 0
end

function proctri.deinit()
	shaderProgram = nil
	collectgarbage()
end

function proctri.resize(width, height)
end

function proctri.update(dt)
end

function proctri.render()
	gl.BindVertexArray(vertexArrays[1])
end

return proctri

--[[
TODO 11/11/2021

I'm going to need to create a linear algebra library for Lua,
or create bindings to the C one I've already created.

I also need to go through the OpenGL header and extract the
values for enums and other value definitions.

Alternatively: Create bindings for a higher-level abstraction
first, then add support for custom render functions later on.

API could be something like, create mesh with particular shader

]]