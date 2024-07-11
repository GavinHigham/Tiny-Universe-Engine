-- local gl = require 'OpenGL'
local glla = require 'glla'
local input = require 'input'
local util, gl = require 'lib/util'.safe()
local trackball = require 'lib/trackball'
local VertexData = require 'lib/vertexdata'
local glsw = util.glsw
local vec2 = glla.vec2
local vec3 = glla.vec3
local vec4 = glla.vec4
local mat3 = glla.mat3
local amat4 = glla.amat4
proctri = {}
local time = 0.0
local screen_width, screen_height
local selected_primitive = 0

--Adapted from http://www.glprogramming.com/red/chapter02.html
local ico_x = 0.525731112119133606
local ico_z = 0.850650808352039932

local function IcoVertexData(s)
	local x, z = ico_x, ico_z
	--TODO: Actually generate the VBO from the icosphere data
	local ico_v = {
		-x,  0, z,  x,  0,  z, -x, 0, -z,  x, 0, -z, 0,  z, x,  0,  z, -x,
		 0, -z, x,  0, -z, -x,  z, x,  0, -z, x,  0, z, -x, 0, -z, -x,  0
	}
	local ico_i = {
		1,0,4,   9,4,0,   9,5,4,   8,4,5,   4,8,1,  10,1,8,  10,8,3,  5,3,8,   3,5,2,  9,2,5,
		3,7,10,  6,10,7,  7,11,6,  0,6,11,  0,1,6,  10,6,1,  0,11,9,  2,9,11,  3,2,7,  11,7,2,
	}
	--Texture coordinates for each vertex of a face. Pairs of faces share a texture.
	local ico_tx = {
		0.0,0.0, 1.0,0.0, 0.0,1.0,  1.0,1.0, 0.0,1.0, 1.0,0.0,  0.0,0.0, 1.0,0.0, 0.0,1.0,  1.0,1.0, 0.0,1.0, 1.0,0.0,
		0.0,0.0, 1.0,0.0, 0.0,1.0,  1.0,1.0, 0.0,1.0, 1.0,0.0,  0.0,0.0, 1.0,0.0, 0.0,1.0,  1.0,1.0, 0.0,1.0, 1.0,0.0,
		0.0,0.0, 1.0,0.0, 0.0,1.0,  1.0,1.0, 0.0,1.0, 1.0,0.0,  0.0,0.0, 1.0,0.0, 0.0,1.0,  1.0,1.0, 0.0,1.0, 1.0,0.0,
		0.0,0.0, 1.0,0.0, 0.0,1.0,  1.0,1.0, 0.0,1.0, 1.0,0.0,  0.0,0.0, 1.0,0.0, 0.0,1.0,  1.0,1.0, 0.0,1.0, 1.0,0.0,
		0.0,0.0, 1.0,0.0, 0.0,1.0,  1.0,1.0, 0.0,1.0, 1.0,0.0,  0.0,0.0, 1.0,0.0, 0.0,1.0,  1.0,1.0, 0.0,1.0, 1.0,0.0,
	}

	local vertices = {}
	for i,v in ipairs(ico_i) do
		table.insert(vertices, ico_v[v*3+1])
		table.insert(vertices, ico_v[v*3+2])
		table.insert(vertices, ico_v[v*3+3])
		table.insert(vertices, ico_tx[(i-1)*2+1])
		table.insert(vertices, ico_tx[(i-1)*2+2])
	end

	return VertexData.VertexData(s).vertices(vertices)--.indices(ico_i)
end

function proctri.init()
	-- util.print_gl_calls = true
	vertexArrays = gl.VertexArrays(1)
	gl.BindVertexArray(vertexArrays[1])

	--[[
	vertexArrays[1]:bind()
	local buffers = gl.Buffers(1)

	--This could be a nice syntax
	buffers[1]:bind(gl.ARRAY_BUFFER):bufferData(
	'vec3 color, float time', {
		1.0, 0.0, 0.0,  0.0,
		0.0, 1.0, 0.0,  0.5,
		0.0, 0.0, 1.0,  1.0
	})

	--Could also have "pack" functions
	i32pack{1.0, 0.0, 0.0, 0.0}
	f32pack{1.0, 0.0, 0.0, 0.0}
	pack(
	'vec3 color, float time', {
		1.0, 0.0, 0.0,  0.0,
		0.0, 1.0, 0.0,  0.5,
		0.0, 0.0, 1.0,  1.0
	})

	vertexArrays[1]:attributes:


	--Ultimately I'll paper over this with the parser project, so maybe it doesn't matter
	--explicit is probably more efficient.

	buffers[1]:bind(gl.ARRAY_BUFFER):bufferData()
	buffers[1]:bufferData(spaceshipModel):bind(gl.ARRAY_BUFFER)

	local buffers = gl.Buffers(1)
	print("#buffers = "..#buffers)
	gl.BindBuffer(gl.ARRAY_BUFFER, buffers[1])
	]]

	local shaders = glsw(io.open('shaders/glsw/atmosphere.glsl', 'r'):read('a'))
	local common = glsw(io.open('shaders/glsw/common.glsl', 'r'):read('a'))
	local vs, log = gl.VertexShader('#version 330\n' .. shaders['vertex.GL33'])
	if log then
		print(log)
	end
	local fs, log = gl.FragmentShader(table.concat({'#version 330',
			shaders['fragment.GL33'], common['utility'], common['noise'],
			common['lighting'], common['debugging']}, '\n'))
	if log then
		print(log)
	end
	local shaderProgram, log = gl.ShaderProgram(vs, fs)
	if log then
		print(log)
	end
	_G.shaderProgram = shaderProgram
	-- print("#shaderProgram = "..#shaderProgram)

	gl.Enable(gl.DEPTH_TEST)
	gl.Enable(gl.PRIMITIVE_RESTART)
	gl.Disable(gl.CULL_FACE)
	gl.ClearDepth(1)
	gl.DepthFunc(gl.LESS)
	gl.PrimitiveRestartIndex(0xFFFFFFFF)

	-- icosphere = VertexData.VertexData('')

	-- local a, b = vec3(1, 2, 3), vec3(4, 5, 6)
	-- -- a.xzy = vec3(10, 11, 12)
	-- -- a.zyx = b.zyy
	-- local c = a:normalize()
	-- print(a)
	-- print(c)
	gl.UseProgram(shaderProgram)
	shaderProgram.resolution = vec2(screen_width or 1024, screen_height or 768)

	local v0 = vec3(0,0,0)
	tri_frame = amat4(mat3.identity(), v0)
	ico_frame = amat4(mat3.identity(), v0)
	sphere_trackball = trackball(tri_frame.t, 5.0)
	sphere_trackball.set_speed(1.0/200.0, 1.0/200.0, 1/10.0)
	eye_frame = amat4(mat3.identity(), vec3(0, 0, 5))
	icovdata = IcoVertexData('vec3 position, vec2 tx')
	-- icovdata.prepareForDraw(shaderProgram)
	gl.BindVertexArray(vertexArrays[1])
	-- icoBuffers = gl.Buffers(2)
	-- gl.BindBuffer(gl.ARRAY_BUFFER, icoBuffers[1])
	-- gl.BufferData(gl.ARRAY_BUFFER, 4*#icoVertices, gl.Pack32f(icoVertices), gl.STATIC_DRAW)
	-- gl.BindBuffer(gl.ELEMENT_ARRAY_BUFFER, icoBuffers[2])
	-- gl.BufferData(gl.ELEMENT_ARRAY_BUFFER, 4*#icoIndices, gl.Pack32i(icoIndices), gl.STATIC_DRAW)
	-- return {vertices = gl.Pack32f(vertices), indices = gl.Pack32i(ico_i)}

	-- vao1 = VertexData.getVertexArray('vec3 hi')
	-- vao2 = VertexData.getVertexArray('vec3 hi')
	-- vao3 = VertexData.getVertexArray('float hello')
	-- vao4 = VertexData.getVertexArray('float hello, vec3 hi')
	-- print(vao1, vao2, vao3, vao4)
	-- vao1, vao2, vao3 = nil, nil, nil
	-- collectgarbage()
	-- collectgarbage()
	-- collectgarbage()

	-- vao1 = VertexData.getVertexArray('float hi')
	-- vao2 = VertexData.getVertexArray('vec3 hi')
	-- vao3 = VertexData.getVertexArray('float hello')
	-- print(vao1, vao2, vao3, vao4)

	return 0
end

function proctri.deinit()
	shaderProgram = nil
	collectgarbage()
end

function proctri.resize(width, height)
	screen_width, screen_height = width, height
	shaderProgram.resolution = vec2(width, height)
end

function proctri.update(dt)
	time = time + dt
	eye_frame.t = eye_frame.t + sphere_trackball.camera.a * vec3(input.scancodeDirectional('D', 'A', 'E', 'Q', 'S', 'W')) * 0.15
	sphere_trackball.step(input.mouseForUI())
	if input.isScancodePressed 'N' or input.isScancodePressed 'M' then
		selected_primitive = selected_primitive + input.scancodeDirectional('M', 'N')
		print('Selected primitive:', selected_primitive)
	end
end

local sqrt = math.sqrt
local function ico_inscribed_radius(edge_len)
	return sqrt(3)*(3+sqrt(5))*edge_len/12.0;
end

function proctri.render()
	-- gl.ClearColor(0.01, 0.22, 0.23, 1.0)
	gl.ClearColor(0,0,0,0)
	gl.Clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT | gl.STENCIL_BUFFER_BIT)
	gl.UseProgram(shaderProgram)
	gl.Enable(gl.DEPTH_TEST)
	--Draw in wireframe if 'z' is held down.
	if input.isKeyDown 'Z' then
		gl.PolygonMode(gl.LINE)
	else
		gl.PolygonMode(gl.FILL)
	end

	shaderProgram.selected_primitive = selected_primitive
	shaderProgram.samples_ab = proctri.meters['Samples along AB'] or 10
	shaderProgram.samples_cp = proctri.meters['Samples along CP'] or 10
	shaderProgram.atmosphere_height = proctri.meters['"Atmosphere Height"'] or 0.02
	shaderProgram.focal_length = proctri.meters['Field of View'] or 1.0/math.tan(math.pi/12)
	shaderProgram.p_and_p = proctri.meters['Poke and Prod Variable'] or 1
	shaderProgram.air_b = vec3(
		proctri.meters['Air Particle Interaction Transmittance R'] or 0.00000519673,
		proctri.meters['Air Particle Interaction Transmittance G'] or 0.0000121427,
		proctri.meters['Air Particle Interaction Transmittance B'] or 0.0000296453)
	shaderProgram.scale_height = proctri.meters['Scale Height'] or 8500.0
	shaderProgram.planet_scale = proctri.meters['Planet Scale'] or 6371000.0
	shaderProgram.resolution = vec2(screen_width or 1024, screen_height or 768)
	shaderProgram.time = time
	shaderProgram.planet = vec4(0,0,0, proctri.meters['Planet Radius'] or 1)
	shaderProgram.ico_scale = (shaderProgram.planet.w + shaderProgram.atmosphere_height) / ico_inscribed_radius(2*ico_x)

	shaderProgram.dir_mat = sphere_trackball.camera.a:transposed()
	shaderProgram.eye_pos = eye_frame.t
	shaderProgram.model_matrix = amat4(mat3.identity(), tri_frame.t)
	shaderProgram.model_view_projection_matrix = proctri.proj_mat * amat4(sphere_trackball.camera.a, eye_frame.t):inversed() * shaderProgram.model_matrix
	icovdata.draw(shaderProgram)

--[[
Nice syntax for setting uniforms would abstract away the type, but still do type checking. Something like:
(Option 1)
shaderProgram.projectionMatrix = projMat
shaderProgram.cameraPos = vec3(3, 4, 5)
shaderProgram.time = 5.4

Could also do:
shaderProgram:uniforms {
	projectionMatrix = projMat,
	cameraPos = vec3(3, 4, 5),
	time = 5.4,
}
shaderProgram:uniforms{time = 3.1}
uniformsTable = shaderProgram:uniforms()
gl.uniforms(shaderProgram){time = 0.0}

Preferred option. It's succinct, and not too confusing if you're familiar with Lua-isms.

(Option 2)
shaderProgram:setUniform("projectionMatrix", projMat)

Sort of Java/Love2D style. Boring, but other options could be built on top of it.

(Option 3)
shaderProgram:uniforms["projectionMatrix"] = projMat
shaderProgram:uniforms.projectionMatrix = projMat
shaderProgram:uniforms = {
	projectionMatrix = projMat,
	cameraPos = vec3(3, 4, 5),
	time = 5.4,
}

This one gets annoying, having to type .uniforms repeatedly, and string args for names are annoying.
The assignment of the table is misleading, because you're not nullifying unreferenced uniforms.
Although, I could make that be an alternative, where if you don't explicitly assign every uniform, it's an error.

--
In all cases, I want to make this step unecessary when I unify shaders with Lua scripting.


Next, I want a way to tweak uniforms with minimal fiddling. Something like:
	shaderProgram.time = tweak{name = 'Time', min = 0, max = 10, style = tweakStyles.default}

	{
		.name = "Planet Radius", .x = 5.0, .y = 0, .min = 0.1, .max = 5.0,
		.target = &at->planet_radius, 
		.style = wstyles, .color = {.fill = {187, 187, 187, 255}, .border = {95, 95, 95, 255}, .font = {255, 255, 255}}
	},
]]
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