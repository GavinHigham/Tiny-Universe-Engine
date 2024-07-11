-- local gl = require 'OpenGL'
local glla = require 'glla'
local input = require 'input'
local util,gl = require 'lib/util'.safe()
local trackball = require 'lib/trackball'
local VertexData = require 'lib/vertexdata'
local glsw = util.glsw
local vec2 = glla.vec2
local vec3 = glla.vec3
local vec4 = glla.vec4
local mat3 = glla.mat3
local amat4 = glla.amat4

planet = {}
local stuff

local time = 0.0
local screen_width, screen_height = 1024, 768
local selected_primitive = 0
local planetShader
local prim_restart_idx = 0xFFFFFFFF
local PlyFileVertexData = VertexData.PlyFileVertexData

local function SpaceshipVertexData()
	return PlyFileVertexData('models/source_models/newship.ply')
end

local function CubeVertexData()
	local r,g,b = 120, 120, 120
	return VertexData.VertexData('vec3 position, vec3 normal, vec3 color')
		.vertices({
			 1,  1, -1, -0.57735029100962, -0.57735033044847,  0.57735018611077, r, g, b,
			-1,  1, -1,  0.57735026918963, -0.57735026918963,  0.57735026918963, r, g, b,
			-1, -1, -1,  0.57735041352697,  0.57734998051467,  0.57735041352713, r, g, b,
			 1, -1,  1, -0.57735003730086,  0.57735054919095, -0.57735022107695, r, g, b,
			-1, -1,  1,  0.57735078528103,  0.57734991925564, -0.57735010303185, r, g, b,
			-1,  1,  1,  0.57735028233568, -0.57735042667341, -0.57735009855974, r, g, b,
			 1, -1, -1, -0.57735035674092,  0.57735013352593,  0.57735031730201, r, g, b,
			 1,  1,  1, -0.57735048793203, -0.57734987114319, -0.57735044849346, r, g, b,
		})
		.indices({0,1,2, 3,4,5, 6,3,7, 2,4,3,2,1,5,5,1,0,6,0,2,7,3,5,0,6,7,6,2,3,4,2,5,7,5,0,}).mode(gl.TRIANGLES)
end

function planet.init()
	-- util.print_gl_calls = true

	local v0 = vec3(0,0,0)
	tri_frame = amat4(mat3.identity(), v0)
	ico_frame = amat4(mat3.identity(), v0)
	sphere_trackball = trackball(tri_frame.t, 5.0)
	sphere_trackball.set_speed(1.0/200.0, 1.0/200.0, 1/10.0)
	eye_frame = amat4(mat3.identity(), vec3(0, 0, 5))


	-- util.print_gl_calls = true
	local common = glsw(io.open('shaders/glsw/common.glsl', 'r'):read('a'))
	local forward = glsw(io.open('shaders/glsw/forward.glsl', 'r'):read('a'))

	forwardShader = assert(gl.ShaderProgram(
		assert(gl.VertexShader[[
		#version 330
		in vec3 position;
		layout(location=1) in vec3 normal;
		in vec3 color;

		uniform float log_depth_intermediate_factor;

		uniform mat4 model_matrix;
		uniform mat4 model_view_normal_matrix;
		uniform mat4 model_view_projection_matrix;
		uniform float hella_time;

		out vec3 fPos;
		out vec3 fNormal;
		out vec3 fColor;

		void main()
		{
			gl_Position = model_view_projection_matrix * vec4(position, 1);
			//gl_Position.z = (log2(max(1e-6, 1.0 + gl_Position.z)) * log_depth_intermediate_factor - 1.0) * gl_Position.w;
			fPos = vec3(model_matrix * vec4(position, 1));
			fNormal = vec3(vec4(normal, 0.0) * model_view_normal_matrix);
			fColor = color;
		}]]),
		assert(gl.FragmentShader(table.concat({
			'#version 330',
			common['lighting'],
			forward['fragment.GL33'],
		}, '\n')))
	))

	-- cubevdatas = {}
	-- shipvdata = SpaceshipVertexData()
	-- roomvdata = PlyFileVertexData('models/source_models/room.ply')
	-- cubevdata = CubeVertexData()
	stuff = {}
	-- table.insert(stuff, SpaceshipVertexData())
	-- table.insert(stuff, SpaceshipVertexData())
	-- table.insert(stuff, SpaceshipVertexData())
	-- table.insert(stuff, SpaceshipVertexData())
	-- table.insert(stuff, SpaceshipVertexData())
	-- table.insert(stuff, PlyFileVertexData('models/source_models/room.ply'))
	-- table.insert(stuff, CubeVertexData())
	-- table.insert(stuff, CubeVertexData())
	-- table.insert(stuff, CubeVertexData())
	-- table.insert(stuff, CubeVertexData())
	-- table.insert(stuff, CubeVertexData())

	planet_instance = require 'lib/planet'(planet)
	return 0
end

function planet.deinit()
	planetShader = nil
end

function planet.resize(width, height)
	screen_width, screen_height = width, height
end

function planet.update(dt)
	time = time + dt
	eye_frame.t = eye_frame.t + sphere_trackball.camera.a * vec3(input.scancodeDirectional('D', 'A', 'E', 'Q', 'S', 'W')) * 0.15
	sphere_trackball.step(input.mouseForUI())
	if input.isScancodePressed 'N' or input.isScancodePressed 'M' then
		selected_primitive = selected_primitive + input.scancodeDirectional('M', 'N')
		print('Selected primitive:', selected_primitive)
	end

	if not addedRoom then
		table.insert(stuff, PlyFileVertexData('models/source_models/room.ply'))
		addedRoom = true
	end
end

function planet.render()
	gl.ClearColor(0,0,0,0)
	gl.DepthFunc(gl.LESS)
	gl.ClearDepth(1)
	gl.Clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT | gl.STENCIL_BUFFER_BIT)
	gl.Enable(gl.CULL_FACE)
	gl.Enable(gl.DEPTH_TEST)
	gl.Enable(gl.PRIMITIVE_RESTART)
	gl.PrimitiveRestartIndex(prim_restart_idx)

	--Draw in wireframe if 'z' is held down.
	if input.isKeyDown 'Z' then
		gl.PolygonMode(gl.LINE)
	else
		gl.PolygonMode(gl.FILL)
	end

	local function forward_draw(shader, vdata, pos)
		gl.UseProgram(shader)
		shader.uLight_pos = vec3(30,30,30)
		shader.uLight_col = vec3(1,1,1)
		shader.uLight_attr = vec4(0.0, 0.0, 1, 5)
		-- print('log depth intermediate factor:', planet.log_depth_intermediate_factor)
		shader.log_depth_intermediate_factor = planet.log_depth_intermediate_factor

		local mm = mat3.identity()
		shader.model_matrix = amat4(mm, pos or tri_frame.t)
		shader.model_view_normal_matrix = amat4(mm:transposed(), vec3(0,0,0))
		shader.model_view_projection_matrix = planet.proj_mat * amat4(sphere_trackball.camera.a, eye_frame.t):inversed() * shader.model_matrix
		shader.hella_time = time
		shader.camera_position = vec3(0,0,0)
		shader.ambient_pass = 0

		vdata.draw(forwardShader)
	end

	-- planet_draw(planetShader, icovdata, vec3(0,0,0))
	if not planet_instance then print 'No planet_instance!' end
	planet_instance.draw()

	local num_stuff = #stuff
	for i,v in ipairs(stuff) do
		forward_draw(forwardShader, v, vec3(10*(i-num_stuff/2), 0, 0))
	end
	-- forward_draw(forwardShader, shipvdata, vec3(-4, 0, 0))
	-- forward_draw(forwardShader, cubevdata, vec3(2, 0, 0))
	-- forward_draw(forwardShader, roomvdata)
end

function planet.onfiledrop(filename)
	print('File was dropped: '..filename)
	if filename:sub(-4, -1) == '.ply' then
		local plyfile = PlyFileVertexData(filename)
		table.insert(stuff, plyfile)
	end
end

return planet