-- local gl = require 'OpenGL'
local glla = require 'glla'
local input = require 'input'
local image = require 'image'
local util,gl = require 'lib/util'.safe()
local meters = require 'lib/meters'
local trackball = require 'lib/trackball'
local VertexData = require 'lib/vertexdata'
local ply = require 'models/parse_ply'
local glsw = util.glsw
local vec2 = glla.vec2
local vec3 = glla.vec3
local vec4 = glla.vec4
local mat3 = glla.mat3
local amat4 = glla.amat4
space = {}
local time = 0.0
local screen_width, screen_height = 1024, 768
local selected_primitive = 0
local planetShader
local prim_restart_idx = 0xFFFFFFFF
local commonVertexData
local dynamicVertexData
local space_scene_shaders_path = 'shaders/space_scene_shaders'
dbg = require 'lib/debug/debugger'
-- local assert = dbg.assert

--Adapted from http://www.glprogramming.com/red/chapter02.html
local ico_x = 0.525731112119133606
local ico_z = 0.850650808352039932
local PlyFileVertexData = VertexData.PlyFileVertexData

local show_depth_mode = {
	none = 0,
	shadow_map = 1,
	shadow_receiver = 2,
}

local function CubeVertexData(color, alt)
--[[

        X-axis (-1) ---- (0) ---- (1)
                                        Z-axis
                       1_____0           / (-1)
          1\           /`   /\          /
            \       5 /__`7/..\ 6      /
             \        \  ,2\  /       / (0)
             0\        \,___\/       /
               \       4     3      /
                \                  / (1)
               -1\
                Y-axis

          (7 is in front, 2 is in the back)
]]

	local r,g,b = 120, 120, 120
	if color then r, g, b = color.x, color.y, color.z end
	if alt == 'wireframe' then
		local r = prim_restart_idx
		return commonVertexData('vec3 position, vec3 color')
			.vertices({
				 1,  1, -1, r, g, b,
				-1,  1, -1, r, g, b,
				-1, -1, -1, r, g, b,
				 1, -1,  1, r, g, b,
				-1, -1,  1, r, g, b,
				-1,  1,  1, r, g, b,
				 1, -1, -1, r, g, b,
				 1,  1,  1, r, g, b,
			})
			.indices({0,1,5,7,0,6,3,4,2,6,r,7,3,r,5,4,r,1,2})
			.mode(gl.LINE_STRIP)
	else
		return commonVertexData('vec3 position, vec3 normal, vec3 color')
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
end

local function CubeVertexDataShadow()
	return commonVertexData('vec3 position')
		.mode(gl.TRIANGLES)
		.vertices({
			 1,  1, -1,
			-1,  1, -1,
			-1, -1, -1,
			 1, -1,  1,
			-1, -1,  1,
			-1,  1,  1,
			 1, -1, -1,
			 1,  1,  1,
		})
		.indices({0,1,2, 3,4,5, 6,3,7, 2,4,3,2,1,5,5,1,0,6,0,2,7,3,5,0,6,7,6,2,3,4,2,5,7,5,0,})
end

local function UVQuad(x, y, w, h, dynamic)
	local fn = dynamic and dynamicVertexData or commonVertexData
	return commonVertexData('vec2 pos, vec2 uv')
		.vertices({
			x,   y,   0, 1,
			x+w, y,   1, 1,
			x,   y+h, 0, 0,
			x+w, y+h, 1, 0
		})
		.mode(gl.TRIANGLE_STRIP)
end

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

	--We don't use an index buffer because vertices have different UV coords per-face
	return commonVertexData('vec3 position, vec2 tx').vertices(vertices)
end

local function LineVertexData(p1, p2, color, color2)
	color = color or vec3(255,0,0)
	color2 = color2 or color
	local vertices = {p1.x, p1.y, p1.z, color.x, color.y, color.z, p2.x, p2.y, p2.z, color2.x, color2.y, color2.z}
	return dynamicVertexData('vec3 position, vec3 color').vertices(vertices).mode(gl.LINES)
end

local sqrt = math.sqrt
local function ico_inscribed_radius(edge_len)
	return sqrt(3)*(3+sqrt(5))*edge_len/12.0;
end

local function planet_draw(shader, vdata, pos, cam)
	gl.UseProgram(shader)
	shader.resolution = vec2(screen_width, screen_height)
	shader.samples_ab = space.meters['Samples along AB'] or 10
	shader.samples_cp = space.meters['Samples along CP'] or 10
	shader.atmosphere_height = space.meters['"Atmosphere Height"'] or 0.02
	shader.focal_length = space.meters['Field of View'] or 1.0/math.tan(math.pi/12)
	shader.p_and_p = space.meters['Poke and Prod Variable'] or 1
	shader.air_b = vec3(
		space.meters['Air Particle Interaction Transmittance R'] or 0.00000519673,
		space.meters['Air Particle Interaction Transmittance G'] or 0.0000121427,
		space.meters['Air Particle Interaction Transmittance B'] or 0.0000296453)
	shader.scale_height = space.meters['Scale Height'] or 8500.0
	shader.planet_scale = space.meters['Planet Scale'] or 6371000.0
	shader.resolution = vec2(screen_width or 1024, screen_height or 768)
	shader.time = time
	shader.planet = vec4(0,0,0, space.meters['Planet Radius'] or 1)
	shader.ico_scale = (shader.planet.w + shader.atmosphere_height) / ico_inscribed_radius(2*ico_x)

	shader.dir_mat = (cam and cam.a and cam.a:transposed()) or sphere_trackball.camera.a:transposed()
	shader.eye_pos = (cam and cam.t) or eye_frame.t
	shader.model_matrix = amat4(mat3.identity(), pos or tri_frame.t)
	shader.model_view_projection_matrix = space.proj_mat * amat4((cam and cam.a) or sphere_trackball.camera.a, (cam and cam.t) or eye_frame.t):inversed() * shader.model_matrix
	vdata.draw(shader)
end

function space.init(reload)
	-- util.print_gl_calls = true
	local atmos = glsw(io.open('shaders/glsw/atmosphere.glsl', 'r'):read('a'))
	local common = glsw(io.open('shaders/glsw/common.glsl', 'r'):read('a'))
	local forward = glsw(io.open('shaders/glsw/forward.glsl', 'r'):read('a'))
	local scene_shaders = require(space_scene_shaders_path)
	planetShader = assert(gl.ShaderProgram(
		assert(gl.VertexShader('#version 330\n' .. atmos['vertex.GL33'])),
		assert(gl.FragmentShader(table.concat({
			'#version 330',
			atmos['fragment.GL33'],
			common['utility'],
			common['noise'],
			common['lighting'],
			common['debugging'],
		}, '\n')))
	))

	forwardShader = assert(gl.ShaderProgram(
		assert(gl.VertexShader(scene_shaders.forwardv)),
		assert(gl.FragmentShader(table.concat({
			'#version 330',
			common['lighting'],
			forward['fragment.GL33'],
		}, '\n')))
	))

	forwardShadowedShader = assert(gl.ShaderProgram(
		assert(gl.VertexShader(scene_shaders.forwardvshadowed)),
		assert(gl.FragmentShader(table.concat({
			'#version 330',
			common['lighting'],
			scene_shaders.forwardfshadowed,
		}, '\n')))
	))

	wireShader = assert(gl.ShaderProgram(
		assert(gl.VertexShader(scene_shaders.wirev)),
		assert(gl.FragmentShader(scene_shaders.wiref))))

	quadShader = assert(gl.ShaderProgram(
		assert(gl.VertexShader(scene_shaders.quadv)),
		assert(gl.FragmentShader(scene_shaders.quadf))))

	quadShadowShader = assert(gl.ShaderProgram(
		assert(gl.VertexShader(scene_shaders.quadv)),
		assert(gl.FragmentShader(scene_shaders.quadshadowf))))

	shadowShader = assert(gl.ShaderProgram(
		assert(gl.VertexShader(scene_shaders.shadowv)),
		assert(gl.FragmentShader(scene_shaders.shadowf))))

	outlineShader = assert(gl.ShaderProgram(
		assert(gl.VertexShader(scene_shaders.outlinev)),
		assert(gl.GeometryShader(scene_shaders.outlineg)),
		assert(gl.FragmentShader(scene_shaders.outlinef))))

	space.meters = meters()
	space.meters.add('Test', 100, 50, 0, 0.5, 1)

	--Init globals
	--Keep state on reload for convenience
	if not reload then
		light_pos = vec3(10,3,3)
		local v0 = vec3(0,0,0)
		tri_frame = amat4(mat3.identity(), v0)
		ico_frame = amat4(mat3.identity(), v0)
		eye_pos = vec3(15, 20, 20)
		sphere_trackball = trackball(tri_frame.t, eye_pos)
		sphere_trackball.set_speed(1.0/200.0, 1.0/200.0, 1/10.0)
		eye_frame = sphere_trackball.camera
	end

	--Init vertex data
	commonVertexData = VertexData.VertexDataGroup()
	dynamicVertexData = VertexData.VertexDataGroup()
	cubevdatas = {}
	quadvdata = UVQuad(0,0,156,48)
	shipvdata = VertexData.FromPlyFile(commonVertexData, 'models/source_models/newship.ply')
	shipoutlinevdata = VertexData.FromPlyFilePosition(commonVertexData, 'models/source_models/newship.ply', 'adjacency')
	roomvdata = VertexData.FromPlyFile(commonVertexData, 'models/source_models/room.ply')
	cubevdata = CubeVertexData()
	icovdata = IcoVertexData()
	debugCube = CubeVertexData(vec3(255, 0, 0), 'wireframe')

	--Init shadows
	shipvdatashadow = shipoutlinevdata
	roomvdatashadow = VertexData.FromPlyFilePosition(commonVertexData, 'models/source_models/room.ply', 'adjacency')
	cubevdatashadow = CubeVertexDataShadow()

	--Init textures
	offscreen = image.Texture(screen_width, screen_height)
	offscreenquad = UVQuad(screen_width/2,screen_height/2,screen_width/2,screen_height/2)

	shadow_tex = image.Texture(1024, 1024, {
		format = 'DEPTH_COMPONENT',
		border_color = {1.0, 1.0, 1.0, 1.0},
		wrap_s = 0x812D,--[['CLAMP_TO_BORDER']]
		wrap_t = 0x812D,--[['CLAMP_TO_BORDER']]
	})

	softshadow_tex = image.Texture(1024, 1024, {
		format = 'DEPTH_COMPONENT',
		border_color = {1.0, 1.0, 1.0, 1.0},
		wrap_s = 0x812D,--[['CLAMP_TO_BORDER']]
		wrap_t = 0x812D,--[['CLAMP_TO_BORDER']]
	})

	local l, r, b, t, n, f = -10, 10, -10, 10, 1, 20
	-- shadowProjection = glla.mat4.projection(math.pi/4, 1, 1, 20)
	shadowProjection = glla.mat4.orthographic(l, r, b, t, n, f)
	orthoDebugScaleMatrix = glla.mat4.orthographic_inverse(l, r, b, t, n, f)


	test_tex = image.fromFile('4x7.png'):toTexture({min_filter = 0x2600, mag_filter = 0x2600})
	grasstex = image.fromFile('grass.png'):toTexture()

	return 0
end

function space.deinit()
	planetShader = nil
	--This will make sure `require(space_scene_shaders_path)` will reload the file when the scene reloads
	package.loaded[space_scene_shaders_path] = nil
	package.loaded['lib/vertexdata'] = nil
end

function space.resize(width, height)
	screen_width, screen_height = width, height
end

function space.update(dt)
	time = time + dt
	local x, y, mouseDown = input.mouseForUI()
	space.meters.mouse(x, y, mouseDown, input.isModifierDown 'shift', input.isModifierDown 'ctrl')
	if input.isScancodeDown ';' then
		manual_light_pos = true
		light_pos = light_pos + vec3(input.scancodeDirectional('D', 'A', 'E', 'Q', 'S', 'W')) * 0.15
		shadowlinevdata = LineVertexData(light_pos, vec3(-4, 0, 0))
	else
		eye_frame.t = eye_frame.t + sphere_trackball.camera.a * vec3(input.scancodeDirectional('D', 'A', 'E', 'Q', 'S', 'W')) * 0.15
	end
	sphere_trackball.step(input.mouseForUI())
	if input.isScancodePressed 'N' or input.isScancodePressed 'M' then
		selected_primitive = selected_primitive + input.scancodeDirectional('M', 'N')
		print('Selected primitive:', selected_primitive)
	end

	if input.isKeyPressed 'V' then
		local show_depth_next = {
			none = 'shadow_map',
			shadow_map = 'shadow_receiver',
			shadow_receiver = 'none',
		}
		show_depth = show_depth_next[show_depth or 'none']
	end

	if not manual_light_pos then
		light_pos = vec3(10*math.sin(time/4.0),3*(math.cos(time/16.0)+2),10*math.cos(time/4.0))
		shadowlinevdata = LineVertexData(light_pos, vec3(-4, 0, 0))
	end

	-- quadvdata = UVQuad(100+100*math.cos(time), 100+100*math.sin(time), 156, 48)
end

function space.render()
	-- gl.ClearColor(0,0,0,0)
	gl.ClearColor(1,1,1,1)
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
		shader.uLight_pos = light_pos or vec3(30,30,30)
		shader.uLight_col = vec3(1,1,1)
		shader.uLight_attr = vec4(0.0, 0.0, 1, 5)
		-- print('log depth intermediate factor:', space.log_depth_intermediate_factor)
		shader.log_depth_intermediate_factor = space.log_depth_intermediate_factor

		local mm = mat3.identity()
		shader.model_matrix = amat4(mm, pos or tri_frame.t)
		shader.model_view_normal_matrix = amat4(mm:transposed(), vec3(0,0,0))
		shader.model_view_projection_matrix = space.proj_mat * amat4(sphere_trackball.camera.a, eye_frame.t):inversed() * shader.model_matrix
		shader.hella_time = time
		shader.camera_position = eye_frame.t
		shader.ambient_pass = false

		vdata.draw(shader)
	end

	--For x and y, scale by 0.5 and add 0.5
	local shadowScaleMat = amat4(mat3(
		0.5, 0.0, 0.0,
		0.0, 0.5, 0.0,
		0.0, 0.0, 0.5), vec3(0.5, 0.5, 0.5))

	local function forward_shadowed_draw(shader, vdata, pos, shadow_map, shadow_matrix)
		gl.UseProgram(shader)
		shader.uLight_pos = light_pos or vec3(30,30,30)
		shader.uLight_col = vec3(1,1,1)
		shader.uLight_attr = vec4(0.0, 0.0, 1, 5)
		-- print('log depth intermediate factor:', space.log_depth_intermediate_factor)
		shader.log_depth_intermediate_factor = space.log_depth_intermediate_factor
		shader.shadow_map = shadow_map
		shader.show_depth = show_depth_mode[show_depth]

		local mm = mat3.identity()
		shader.model_matrix = amat4(mm, pos or tri_frame.t)
		shader.model_view_normal_matrix = amat4(mm:transposed(), vec3(0,0,0))
		local vm = amat4(sphere_trackball.camera.a, eye_frame.t):inversed()
		shader.model_view_projection_matrix = space.proj_mat * vm * shader.model_matrix
		shader.shadow_view_projection_matrix = shadowScaleMat * shadowProjection * shadow_matrix:inversed() * shader.model_matrix
		shader.hella_time = time
		shader.camera_position = eye_frame.t
		shader.ambient_pass = false

		vdata.draw(shader)
	end

	local function shadow_draw(shader, vdata, pos, cam)
		gl.UseProgram(shader)
		shader.model_view_projection_matrix = shadowProjection
			* (cam or amat4(sphere_trackball.camera.a, eye_frame.t)):inversed()
			* amat4(mat3.identity(), pos or tri_frame.t)
		vdata.draw(shader)
	end

	local function drawUVQuad(shader, vdata, tex)
		gl.UseProgram(shader)
		gl.Disable(gl.DEPTH_TEST)
		gl.Disable(gl.CULL_FACE)
		shader.screen_res = vec2(screen_width, screen_height)
		shader.tex = tex
		vdata.draw(shader)
	end

	planet_draw(planetShader, icovdata, vec3(0,0,0))
	-- forward_draw(forwardShader, cubevdata, vec3(2, 0, 0))
	-- offscreen:drawTo(function()
	-- 	gl.ClearColor(0,0,0,1)
	-- 	gl.Clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT)
	-- 	-- forward_draw(forwardShader, shipvdata, vec3(-4, 0, 0))
	-- 	-- forward_draw(forwardShader, cubevdata, vec3(2, 0, 0))
	-- 	-- forward_draw(forwardShader, roomvdata)
	-- 	planet_draw(planetShader, icovdata, vec3(0,0,0))
	-- end)

	local shadowCamera = amat4(mat3.lookat(light_pos, vec3(-4, 0, 0), vec3(0, 1, 0)), light_pos)
	local ship_pos = vec3(-4, 0, math.sin(time)*2)

	shadow_tex:drawTo(function()
		if input.isKeyDown 'Z' then
			gl.PolygonMode(gl.LINE)
		else
			gl.PolygonMode(gl.FILL)
		end
		gl.ClearColor(0,0,0,1)
		gl.Clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT)
		gl.DepthFunc(gl.LESS)

		shadow_draw(shadowShader, shipvdatashadow, ship_pos, shadowCamera)
		-- forward_draw(forwardShader, roomvdata, vec3(0,0,0))
		-- forward_draw(forwardShader, shipvdata, vec3(-4, 0, 0))
		-- planet_draw(planetShader, icovdata, vec3(0,0,0), shadowCamera)
		shadow_draw(shadowShader, cubevdatashadow, vec3(2, 0, 0), shadowCamera)
		shadow_draw(shadowShader, roomvdatashadow, vec3(0, 0, 0), shadowCamera)
	end)

	softshadow_tex:drawTo(function()
		if input.isKeyDown 'Z' then
			gl.PolygonMode(gl.LINE)
		else
			gl.PolygonMode(gl.FILL)
		end
		gl.ClearColor(0,0,0,1)
		gl.Clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT)
		gl.DepthFunc(gl.LESS)

		shadow_draw(shadowShader, shipvdatashadow, ship_pos, shadowCamera)
		shadow_draw(shadowShader, roomvdatashadow, vec3(0, 0, 0), shadowCamera)
	end)

	gl.DepthFunc(gl.LESS)
	forward_shadowed_draw(forwardShadowedShader, roomvdata, nil, shadow_tex:active(0), shadowCamera)
	forward_shadowed_draw(forwardShadowedShader, shipvdata, ship_pos, shadow_tex:active(0), shadowCamera)
	forward_shadowed_draw(forwardShadowedShader, cubevdata, vec3(2, 0, 0), shadow_tex:active(0), shadowCamera)
	-- forward_shadowed_draw(forwardShadowedShader, shipvdata, vec3(-4, 0, 0), shadow_map, shadowCamera)

	gl.PolygonMode(gl.FILL)
	drawUVQuad(quadShadowShader, offscreenquad, softshadow_tex:active(0))

	-- drawUVQuad(quadShader, quadvdata, test_tex:active(0))
	-- drawUVQuad(quadShader, quadvdata, grasstex:active(1), vec2(128, 128))
	-- drawUVQuad(quadShader, offscreenquad, offscreen:active(1))
	-- for i,v in ipairs(dropped_images or {}) do
	-- 	drawUVQuad(quadShader, v[2], v[1]:active(2))
	-- end

	local function drawLine(shader, vdata)
		gl.UseProgram(shader)
		shader.model_view_projection_matrix = space.proj_mat * amat4(sphere_trackball.camera.a, eye_frame.t):inversed()
		vdata.draw(shader)
	end
	drawLine(wireShader, shadowlinevdata)

	local function drawFrustum(shader, vdata)
		gl.UseProgram(shader)
		gl.PolygonMode(gl.LINE)
		shader.model_view_projection_matrix = space.proj_mat *
			amat4(sphere_trackball.camera.a, eye_frame.t):inversed() *
			shadowCamera * orthoDebugScaleMatrix
		vdata.draw(shader)
		gl.PolygonMode(gl.FILL)
	end
	drawFrustum(wireShader, debugCube)

	local function forward_draw_outline(shader, vdata, pos)
		gl.UseProgram(shader)

		local mm = mat3.identity()
		shader.uOrigin = light_pos
		shader.model_matrix = amat4(mm, pos or tri_frame.t)
		shader.model_view_normal_matrix = amat4(mm:transposed(), vec3(0,0,0))
		shader.model_view_projection_matrix = space.proj_mat * amat4(sphere_trackball.camera.a, eye_frame.t):inversed() * shader.model_matrix

		vdata.draw(shader)
	end

	forward_draw_outline(outlineShader, shipoutlinevdata, ship_pos)
end

function space.onfiledrop(filename)
	print('File was dropped: '..filename)
	if filename:sub(-4, -1) == '.png' then
		local img = image.fromFile(filename):toTexture()
		local w, h = img:getSize()
		left_edge = left_edge or 0
		local quad = UVQuad(left_edge,0,w,h)
		left_edge = left_edge + w
		dropped_images = dropped_images or {}
		table.insert(dropped_images, {img, quad})
	end
end

return space