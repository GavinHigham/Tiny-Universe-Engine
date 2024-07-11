-- local gl = require 'OpenGL'
local glla = require 'glla'
local input = require 'input'
local util,gl = require 'lib/util'.safe()
local VertexData = require 'lib/vertexdata'
local trackball = require 'lib/trackball'
local ply = require 'models/parse_ply'
local glsw = util.glsw
local vec2 = glla.vec2
local vec3 = glla.vec3
local vec4 = glla.vec4
local mat3 = glla.mat3
local amat4 = glla.amat4
generative_tree = {}
local time = 0.0
local screen_width, screen_height = 1024, 768
local selected_primitive = 0
local planetShader
local prim_restart_idx = 0xFFFFFFFF

local PlyFileVertexData = vertexdata.PlyFileVertexData

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

local function branch_fn_1(parent, depth, branchIndex)
	local root = vec3.lerp(parent.tip, parent.root, branchIndex / 7)
	local tip_radius = 17.0/(depth*depth) * 3.0/branchIndex
	local tip = root + vec3(
		(math.random()-.5)*tip_radius,
		(math.random()-.2)*tip_radius,
		(math.random()-.5)*tip_radius)

	local tip_dist_from_origin = tip:dist(vec3(0,0,0))
	if not furthest_v or tip_dist_from_origin > furthest_v then
		furthest_v = tip_dist_from_origin
	end
	return {parent = parent, root = root, tip = tip, children = {}, depth = depth}
end

local function GenerativeTree(branchFn)
	local treeSkeleton = {}
	--[[Simple tree will consist of branches, which start partway along a parent branch
	The tree skeleton tree will be walked at the end, generating the actual skeleton vertex positions
	The skin function will then create a skin around the skeleton, giving the branches thickness

	Branches should appear a configurable distance along their parent branch, rotated 1 phi-th around from previous
	]]
	local trunk = {root = vec3(0,0,0), tip = vec3(4, 20, 3), children = {}, depth = 0}
	for i = 1,5 do
		local cb = branchFn(trunk, 1, i)
		table.insert(trunk.children, cb)
		for j = 1,3 do
			table.insert(cb.children, branchFn(cb, 2, j))
		end
	end

	local function insert_vertex(t, position, normal, color)
		table.insert(t, position.x)
		table.insert(t, position.y)
		table.insert(t, position.z)
		table.insert(t, normal.x)
		table.insert(t, normal.y)
		table.insert(t, normal.z)
		table.insert(t, color.x)
		table.insert(t, color.y)
		table.insert(t, color.z)
	end

	function treeSkeleton.VertexData()
		--Make a triangular prism around each branch, except the trunk, which will be a hexagonal prism
		local color = vec3(0x77, 0x5c, 0x23)
		local vertices = {}
		local indices = {}
		local base = 0
		function insert_branch_vertices(branch, num_facets)
			local rot = mat3.lookat(branch.root, branch.tip, vec3(0,1,0))
			-- local rot = mat3.identity()
			local len = branch.root:dist(branch.tip)
			local radius = 1
			if branch.depth == 0 then
				--radius = 6
				num_facets = 6
			end
			for i=1,num_facets do
				local theta = 2.0 * math.pi * (i/num_facets)
				local c, s = math.cos(theta) * radius, math.sin(theta) * radius
				local root_pos = rot * (vec3(c, s, 0)) + branch.root
				local tip_pos = rot * (vec3(c, s, 0)) + branch.tip
				local normal = (root_pos - branch.root):normalize()
				insert_vertex(vertices, root_pos, normal, color)
				insert_vertex(vertices, tip_pos, normal, color)
				table.insert(indices, base+(i*2)-2)
				table.insert(indices, base+(i*2)-1)
			end
			table.insert(indices, base)
			table.insert(indices, base+1)
			base = base + num_facets * 2
			table.insert(indices, prim_restart_idx)
		end

		--Push each branch onto a stack, calculate its verts
		local stack = {trunk}
		while #stack >= 1 do
			local branch = table.remove(stack)
			table.move(branch.children, 1, #branch.children, #stack+1, stack)
			insert_branch_vertices(branch, 6)
		end

		-- for i = 0,#vertices/9-1 do
		-- 	print(table.unpack(table.move(vertices, i*9+1, (i+1)*9, 1, {})))
		-- end

		-- for i = 0,#indices/9-1 do
		-- 	print(table.unpack(table.move(indices, i*9+1, (i+1)*9, 1, {})))
		-- end

		return VertexData.VertexData('vec3 position, vec3 normal, vec3 color')
			.vertices(vertices).indices(indices).mode(gl.TRIANGLE_STRIP)
	end

	return treeSkeleton
end

local function TreeVertexData(seed)
	local function insert_vertex(t, position, normal, color)
		table.insert(t, position.x)
		table.insert(t, position.y)
		table.insert(t, position.z)
		table.insert(t, normal.x)
		table.insert(t, normal.y)
		table.insert(t, normal.z)
		table.insert(t, color.x)
		table.insert(t, color.y)
		table.insert(t, color.z)
	end

	local function insert_branch_vertexdata(vertices, indices, base, color, root, tip, root_radius, tip_radius, num_facets)
		local rot = mat3.lookat(root, tip, vec3(0,1,0))
		--Change these based on branch depth later
		root_radius = root_radius or 1
		tip_radius = tip_radius or 1
		num_facets = num_facets or 6

		for i=1,num_facets do
			local theta = 2.0 * math.pi * (i/num_facets)
			local c, s = math.cos(theta), math.sin(theta)
			local root_pos = rot * vec3(c*root_radius, s*root_radius, 0) + root
			local tip_pos = rot * vec3(c*tip_radius, s*tip_radius, 0) + tip
			if root_pos.x ~= root_pos.x then error('NaN detected!') end
			local normal = (root_pos - root):normalize()
			insert_vertex(vertices, root_pos, normal, color)
			insert_vertex(vertices, tip_pos, normal, color)
			table.insert(indices, base+(i*2)-2)
			table.insert(indices, base+(i*2)-1)
		end
		table.insert(indices, base)
		table.insert(indices, base+1)
		table.insert(indices, prim_restart_idx)
		return base + num_facets * 2
	end

	math.randomseed(seed or 0)

	local function new_branch(parent_root, parent_tip, depth, branch_number)
		local tip_dist = 30.0/(depth) + 6.0/branch_number
		local root = vec3.lerp(parent_tip, parent_root, branch_number / 7)
		local tip = root + vec3(
			(math.random()-.5)*tip_dist,
			(math.random()-.2)*tip_dist,
			(math.random()-.5)*tip_dist)
		return root,tip
	end

	--Make a triangular prism around each branch, except the trunk, which will be a hexagonal prism
	local color = vec3(0x77, 0x5c, 0x23)
	local vertices = {}
	local indices = {}
	local trunk_root, trunk_tip = vec3(0,0,0), vec3(2,20,4)
	local base = insert_branch_vertexdata(vertices, indices, 0, color, trunk_root, trunk_tip, 3, .2, 8)

	for i = 1,5 do
		local branch_root, branch_tip = new_branch(trunk_root, trunk_tip, 1, i)
		base = insert_branch_vertexdata(vertices, indices, base, color, branch_root, branch_tip, 1.2, .1)
		for j = 1,3 do
			local root, tip = new_branch(branch_root, branch_tip, 2, j)
			base = insert_branch_vertexdata(vertices, indices, base, color, root, tip, .7, .05)
		end
	end

	-- for i = 0,#vertices/9-1 do
	-- 	print(table.unpack(table.move(vertices, i*9+1, (i+1)*9, 1, {})))
	-- end

	-- for i = 0,#indices/9-1 do
	-- 	print(table.unpack(table.move(indices, i*9+1, (i+1)*9, 1, {})))
	-- end

	return VertexData.VertexData('vec3 position, vec3 normal, vec3 color')
		.vertices(vertices).indices(indices).mode(gl.TRIANGLE_STRIP)
end


function generative_tree.init()
	-- util.print_gl_calls = true
	local atmos = glsw(io.open('shaders/glsw/atmosphere.glsl', 'r'):read('a'))
	local common = glsw(io.open('shaders/glsw/common.glsl', 'r'):read('a'))
	local forward = glsw(io.open('shaders/glsw/forward.glsl', 'r'):read('a'))
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

	local v0 = vec3(0,0,0)
	tri_frame = amat4(mat3.identity(), v0)
	ico_frame = amat4(mat3.identity(), v0)
	sphere_trackball = trackball(tri_frame.t, 5.0)
	sphere_trackball.set_speed(1.0/200.0, 1.0/200.0, 1/10.0)
	eye_frame = amat4(mat3.identity(), vec3(0, 0, 5))

	cubevdatas = {}
	shipvdata = SpaceshipVertexData()
	roomvdata = PlyFileVertexData('models/source_models/room.ply')
	cubevdata = CubeVertexData()
	treevdata = TreeVertexData()
	print('Furthest V:', furthest_v)
	return 0
end

function generative_tree.deinit()
	planetShader = nil
end

function generative_tree.resize(width, height)
	screen_width, screen_height = width, height
end

function generative_tree.update(dt)
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

function generative_tree.render()
	-- gl.ClearColor(0,0,0,0)
	gl.ClearColor(0.5,0.5,0.5,1)
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

	local function planet_draw(shader, vdata, pos)
		gl.UseProgram(shader)
		shader.resolution = vec2(screen_width, screen_height)
		shader.samples_ab = generative_tree.meters['Samples along AB'] or 10
		shader.samples_cp = generative_tree.meters['Samples along CP'] or 10
		shader.atmosphere_height = generative_tree.meters['"Atmosphere Height"'] or 0.02
		shader.focal_length = generative_tree.meters['Field of View'] or 1.0/math.tan(math.pi/12)
		shader.p_and_p = generative_tree.meters['Poke and Prod Variable'] or 1
		shader.air_b = vec3(
			generative_tree.meters['Air Particle Interaction Transmittance R'] or 0.00000519673,
			generative_tree.meters['Air Particle Interaction Transmittance G'] or 0.0000121427,
			generative_tree.meters['Air Particle Interaction Transmittance B'] or 0.0000296453)
		shader.scale_height = generative_tree.meters['Scale Height'] or 8500.0
		shader.planet_scale = generative_tree.meters['Planet Scale'] or 6371000.0
		shader.resolution = vec2(screen_width or 1024, screen_height or 768)
		shader.time = time
		shader.planet = vec4(0,0,0, generative_tree.meters['Planet Radius'] or 1)
		shader.ico_scale = (shader.planet.w + shader.atmosphere_height) / ico_inscribed_radius(2*ico_x)

		shader.dir_mat = sphere_trackball.camera.a:transposed()
		shader.eye_pos = eye_frame.t
		shader.model_matrix = amat4(mat3.identity(), pos or tri_frame.t)
		shader.model_view_projection_matrix = generative_tree.proj_mat * amat4(sphere_trackball.camera.a, eye_frame.t):inversed() * shader.model_matrix
		vdata.draw(shader)
	end

	local function forward_draw(shader, vdata, pos)
		gl.UseProgram(shader)
		shader.uLight_pos = vec3(30,30,30)
		shader.uLight_col = vec3(1,1,1)
		shader.uLight_attr = vec4(0.0, 0.0, 1, 5)
		-- print('log depth intermediate factor:', generative_tree.log_depth_intermediate_factor)
		shader.log_depth_intermediate_factor = generative_tree.log_depth_intermediate_factor

		local mm = mat3.identity()
		shader.model_matrix = amat4(mm, pos or tri_frame.t)
		shader.model_view_normal_matrix = amat4(mm:transposed(), vec3(0,0,0))
		shader.model_view_projection_matrix = generative_tree.proj_mat * amat4(sphere_trackball.camera.a, eye_frame.t):inversed() * shader.model_matrix
		shader.hella_time = time
		shader.camera_position = vec3(0,0,0)
		shader.ambient_pass = 0

		vdata.draw(forwardShader)
	end

	forward_draw(forwardShader, cubevdata, vec3(2, 0, -5))
	forward_draw(forwardShader, treevdata, vec3(2, 0, 0))
end

return generative_tree