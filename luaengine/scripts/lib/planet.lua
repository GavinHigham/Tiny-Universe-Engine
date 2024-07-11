local util,gl = require 'lib/util'.safe()
local glla = require 'glla'
local VertexData = require 'lib/vertexdata'
local glsw = util.glsw
local vec2 = glla.vec2
local vec3 = glla.vec3
local vec4 = glla.vec4
local mat3 = glla.mat3
local amat4 = glla.amat4

--Adapted from http://www.glprogramming.com/red/chapter02.html
local ico_x = 0.525731112119133606
local ico_z = 0.850650808352039932
local ico_vdata
local planet_shader

local sqrt = math.sqrt
local function ico_inscribed_radius(edge_len)
	return sqrt(3)*(3+sqrt(5))*edge_len/12.0;
end

local function IcoVertexData()
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
	return VertexData.VertexData('vec3 position, vec2 tx').vertices(vertices)
end

local atmosphereFnDecl = --[[glsl]][[
#define OVERRIDE_ATMOSPHERE
//Atmosphere intersection nearest camera, other atmosphere intersection, planet, light position, light intensity
vec3 atmosphere(vec3 ro, vec3 rd, vec4 abcd, float s, vec4 pl, float rA, vec3 l, vec3 lI, float planet_scale_factor, vec2 samples);]]

local atmosphereFn = atmosphereFnDecl:sub(1, -2) .. '\n' .. --[[glsl]][[
{
	//Number of sample points P we consider along AB
	float abSamples = samples[0];
	//Number of sample points Q we consider along CP
	float cpSamples = samples[1];
	//"scale height" of Earth for Rayleigh scattering
	float H = scale_height / planet_scale_factor;
	//Fraction of energy lost to scattering after one particle collision at sea level.
	vec3 b = air_lambda * planet_scale_factor;//beta(air_lambda);
	//Length of a sample segment on AB
	float d_ab = distance(abcd.x, abcd.y) / abSamples;

	//Sum of optical depth along AB
	float abSum = 0.0;
	vec3 T = vec3(0.0);

	for (int i = 0; i < abSamples; i++) {
		//Sample point P along the view ray
		vec3 P = ro + rd*mix(abcd[0], abcd[1], (float(i)+0.5)/abSamples);
		//Altitude of P
		float hP = max(distance(pl.xyz, P) - pl.w, 0.0);
		float odP = exp(-hP/H) * d_ab;
		abSum += odP;

		//Find intersections of sunlight ray with atmosphere sphere
		vec3 lro = l;
		vec3 lrd = normalize(P-l);
		vec3 si = sphereIntersection(lro, lrd, vec4(pl.xyz, rA));
		//Point C, where a ray of light travelling towards P from the sun first strikes the atmosphere
		vec3 C = lro + lrd*si.y;
		//Length of a sample segment on CP
		float d_cp = distance(P, C)/cpSamples;
		//Sum of optical depth along CP
		float cpSum = 0.0;
		//PERF(Gavin): Can I find hQ at each sample point in a cheaper way?
		for (int j = 0; j < cpSamples; j++) {
			//Sample point Q along CP
			vec3 Q = mix(P, C, (float(j)+0.5)/cpSamples);
			//Altitude of Q
			float hQ = distance(pl.xyz, Q) - pl.w;

			cpSum += exp(-hQ/H);
			if (hQ <= 0) {
				cpSum = 1e+20;
				// cpSum = 0.0;
				break;
			}
		}

		//Need to scale by d_cp here instead of outside the loop because d_cp varies with the length of CP
		float odCP = cpSum * d_cp;
		T += exp(-b * (abSum + odCP + odP)); // Moved "* d_ab;" outside the loop for performance
	}
	// return TurboColormap(T.x);
	vec3 I = lI * b * gamma(dot(rd, normalize(l-pl.xyz))) * (T * d_ab);
	return I;
}
]]

function planet_shader_init(shader, config)
	gl.UseProgram(shader)
	shader.samples_ab = config.meters['Samples along AB'] or 10
	shader.samples_cp = config.meters['Samples along CP'] or 10
	shader.atmosphere_height = config.meters['"Atmosphere Height"'] or 0.02
	shader.focal_length = config.meters['Field of View'] or 1.0/math.tan(math.pi/12)
	shader.p_and_p = config.meters['Poke and Prod Variable'] or 1
	shader.air_b = vec3(
		config.meters['Air Particle Interaction Transmittance R'] or 0.00000519673,
		config.meters['Air Particle Interaction Transmittance G'] or 0.0000121427,
		config.meters['Air Particle Interaction Transmittance B'] or 0.0000296453)
	shader.scale_height = config.meters['Scale Height'] or 8500.0
	shader.planet_scale = config.meters['Planet Scale'] or 6371000.0
	shader.resolution = vec2(screen_width or 1024, screen_height or 768)
	shader.time = time
	shader.planet = vec4(0,0,0, config.meters['Planet Radius'] or 1)
	shader.ico_scale = (shader.planet.w + shader.atmosphere_height) / ico_inscribed_radius(2*ico_x)
	-- dir_mat, eye_pos, model_matrix, and model_view_projection_matrix still need to be set per-frame.
end

function PlanetShader(config)
	config = config or {meters={}} --If config not provided, defaults will be used.

	local atmos = glsw(io.open('shaders/glsw/atmosphere.glsl', 'r'))
	local common = glsw(io.open('shaders/glsw/common.glsl', 'r'))
	local shader = assert(gl.ShaderProgram(
		assert(gl.VertexShader('#version 330\n' .. atmos['vertex.GL33'])),
		assert(gl.FragmentShader(table.concat({
			'#version 330',
			atmosphereFnDecl,
			atmos['fragment.GL33'],
			atmosphereFn,
			common['utility'],
			common['noise'],
			common['lighting'],
			common['debugging'],
		}, '\n')))
	))

	planet_shader_init(shader, config)
	return shader
end

function planet_draw(shader, vdata, pos, config)
	gl.UseProgram(shader)
	shader.dir_mat = sphere_trackball.camera.a:transposed()
	shader.eye_pos = eye_frame.t
	shader.model_matrix = amat4(mat3.identity(), pos or tri_frame.t)
	shader.model_view_projection_matrix = config.proj_mat * amat4(sphere_trackball.camera.a, eye_frame.t):inversed() * shader.model_matrix
	vdata.draw(shader)
end

return function(planet_scene)
	-- Make sure vdata and shader are initialized
	ico_vdata = ico_vdata or IcoVertexData()
	planet_shader = planet_shader or PlanetShader()

	local planet = {}
	planet.draw = function()
		-- shader_init(planet_shader, planet_scene)
		planet_draw(planet_shader, ico_vdata, planet.pos or vec3(0,0,0), planet_scene)
	end

	return planet
end

--[[
TODO 2024-04-02

Take this stuff and repackage it into something class-like, so I can make
drawable instances of different planets with differnte encapsulated attributes.

...

Okay I sort of hacked that together. I still want to make the config values be pulled
once and then reused (but overrideable per-instance).

]]
