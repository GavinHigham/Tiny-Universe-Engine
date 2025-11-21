if false then
	return nil
end

local common = {
	triangular_atlas = --[[glsl]][[vec2 atlas_position(float cell_index, int numrows, int numcols)
	{
		float bx = 1.0/numcols;
		float by = 1.0/numrows;
		float row = floor(cell_index / (numrows * 2.0));
		float col = int(floor(cell_index/2.0)) % numcols;

		//TODO: Check the winding?
		int v = gl_VertexID % 3;
		switch(v) {
		case 0:
			return vec2(col*bx, (row+1)*by);
			break;
		case 1:
			return vec2((col+1)*bx, row*by);
			break;
		case 2:
			if (int(cell_index) % 2 == 0)
				return vec2(col*bx, row*by);
			else
				return vec2((col+1)*bx, (row+1)*by);
			break;
		}
	}]]
}

local shaders = {

quadv = --[[glsl]][[
	#version 330
	in vec2 pos;
	in vec2 uv;

	uniform vec2 screen_res = vec2(640, 480);

	out vec2 position;
	out vec2 uv_coord;

	void main()
	{
		position = pos;
		vec2 p = (pos * 2 / screen_res) - 1.0;
		p.y = -p.y;
		uv_coord = uv;
		gl_Position = vec4(p, 0, 1);
	}]],


quadf = --[[glsl]][[
	#version 330
	uniform sampler2D tex;
	//uniform sampler2D font_tex;
	//uniform bool textured = false;

	in vec2 position;
	in vec2 uv_coord;
	out vec4 LFragment;

	void main() {
		vec4 color = texture(tex, uv_coord);
		if (color.a == 0.0)
			discard;
		//LFragment = color;
		LFragment = vec4(color.rgb, 1);
	}]],


--shader that just reads from the depth texture to view it
quadshadowf = --[[glsl]][[
	#version 330
	uniform sampler2D tex;

	in vec2 position;
	in vec2 uv_coord;
	out vec4 LFragment;

	float near_plane = 1.0;
	float far_plane = 20.0;

	//float LinearizeDepth(float depth)
	//{
	//	float z = depth * 2.0 - 1.0; // Back to NDC
	//	return (2.0 * near_plane * far_plane) / (far_plane + near_plane - z * (far_plane - near_plane));
	//}

	void main() {
		float depth = texture(tex, uv_coord).r;
		LFragment = vec4(vec3(depth), 1.0);
	}]],


wirev = --[[glsl]][[
	#version 330
	in vec3 position;
	in vec3 color;

	uniform mat4 model_view_projection_matrix;

	out vec3 fColor;

	void main()
	{
		gl_Position = model_view_projection_matrix * vec4(position, 1);
		fColor = color;
	}]],


wiref = --[[glsl]][[
	#version 330
	in vec3 fColor;
	out vec4 LFragment;

	void main()
	{
		LFragment = vec4(fColor, 1);
	}]],


outlinev = --[[glsl]][[
	#version 330

	in vec3 position;

	uniform mat4 model_matrix;
	uniform mat4 model_view_projection_matrix;
	uniform mat4 model_view_normal_matrix;

	out vec3 gPos;

	void main()
	{
		gl_Position = model_view_projection_matrix * vec4(position, 1);
		vec4 something = vec4(0.0);
		something = (model_view_normal_matrix * something) * 0.0;
		gPos = vec3(model_matrix * vec4(position, 1)) + something.xyz;
	}]],

outlineg = --[[glsl]][[
	#version 330 core
	layout (triangles_adjacency) in;
	layout (line_strip, max_vertices = 6) out;

	uniform vec3 uOrigin;

	in vec3 gPos[6];
	vec4 z_nudge = vec4(0, 0, 0.1, 0);

	void EmitSegment(int StartIndex, int EndIndex)
	{
		gl_Position = gl_in[StartIndex].gl_Position + z_nudge;
		EmitVertex();
		gl_Position = gl_in[EndIndex].gl_Position + z_nudge;
		EmitVertex();
		EndPrimitive();
	}

	void main() {
		vec3 e1 = gPos[2] - gPos[0];
		vec3 e2 = gPos[4] - gPos[0];
		vec3 e3 = gPos[1] - gPos[0];
		vec3 e4 = gPos[3] - gPos[2];
		vec3 e5 = gPos[4] - gPos[2];
		vec3 e6 = gPos[5] - gPos[0];
		vec3 normal = cross(e1, e2);
		vec3 LightDir = uOrigin - gPos[0];
		if (dot(normal, LightDir) >= 0) {

			normal = cross(e3,e1);

			if (dot(normal, LightDir) <= 0) {
				EmitSegment(0, 2);
			}

			normal = cross(e4,e5);
			LightDir = uOrigin - gPos[2];

			if (dot(normal, LightDir) <=0) {
				EmitSegment(2, 4);
			}

			normal = cross(e2,e6);
			LightDir = uOrigin - gPos[4];

			if (dot(normal, LightDir) <= 0) {
				EmitSegment(4, 0);
			}
		}
	}]],


outlinef = --[[glsl]][[
	#version 330

	out vec4 LFragment;

	void main() {
		LFragment = vec4(1.0, 0.0, 0.0, 1.0);
	}]],


forwardv = --[[glsl]][[
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
		}]],


shadowv = --[[glsl]][[
		#version 330
		in vec3 position;

		uniform mat4 model_view_projection_matrix;

		void main()
		{
			gl_Position = model_view_projection_matrix * vec4(position, 1);
		}]],

softshadowv = --[[glsl]][[
		#version 330
		in vec3 position;
		out vec3 pos;
		uniform mat4 model_matrix;

		void main()
		{
			//Move vertices from local space into world space
			pos = (model_matrix * vec4(position, 1.0)).xyz;
		}]],

softshadowg = --[[glsl]][[
#version 330 core
layout (triangles_adjacency) in;
layout (triangle_strip, max_vertices = 18) out;

uniform vec3 gLightPos = vec3(0, 0, 0);
uniform float dilation = 0;

in vec3 pos[6];
uniform mat4 view_projection_matrix;

float EPSILON = 0.0001;

// Emit a quad triangle strip
void quad(vec3 a, vec3 b, vec3 c, vec3 d)
{
	gl_Position = view_projection_matrix * vec4(a, 1.0);
	EmitVertex();
	gl_Position = view_projection_matrix * vec4(b, 1.0);
	EmitVertex();
	gl_Position = view_projection_matrix * vec4(c, 1.0);
	EmitVertex();
	gl_Position = view_projection_matrix * vec4(d, 1.0);
	EmitVertex();
	EndPrimitive(); 
}

void main() {
	vec3 light_pos = gLightPos;
	//Edges needed to find normals of face and adjacent faces
	//0, 2, 4 are vertices of the primary face, 1, 3, 5 vertices of adjacent faces
	vec3 e1 = pos[2] - pos[0];
	vec3 e2 = pos[4] - pos[0];
	vec3 e3 = pos[1] - pos[0];
	vec3 e4 = pos[3] - pos[2];
	vec3 e5 = pos[4] - pos[2];
	vec3 e6 = pos[5] - pos[0];
	vec3 normal = cross(e1, e2); //Primary face normal.
	vec3 lv0 = normalize(light_pos - pos[0]); //From vertex 0 to the light
	vec3 lv2 = normalize(light_pos - pos[2]); //From vertex 2 to the light
	vec3 lv4 = normalize(light_pos - pos[4]); //From vertex 4 to the light
	//Vertices scootched slightly away from the light (for caps)
	//vec3 lp0 = (pos[0] - lv0 * EPSILON);
	//vec3 lp2 = (pos[2] - lv2 * EPSILON);
	//vec3 lp4 = (pos[4] - lv4 * EPSILON);
	//If the primary face faces the light
	if (dot(normal, lv0) > 0) {
		//Front cap
		//gl_Position = view_projection_matrix * vec4(lp0, 1.0);
		gl_Position = view_projection_matrix * vec4(pos[0], 1.0);
		EmitVertex();
		//gl_Position = view_projection_matrix * vec4(lp2, 1.0);
		gl_Position = view_projection_matrix * vec4(pos[2], 1.0);
		EmitVertex();
		//gl_Position = view_projection_matrix * vec4(lp4, 1.0);
		gl_Position = view_projection_matrix * vec4(pos[4], 1.0);
		EmitVertex();
		EndPrimitive();

		float extrusion = 1000.0;

		gl_Position = view_projection_matrix * vec4(pos[0] - lv0 * extrusion, 1.0);
		EmitVertex();
		gl_Position = view_projection_matrix * vec4(pos[2] - lv2 * extrusion, 1.0);
		EmitVertex();
		gl_Position = view_projection_matrix * vec4(pos[4] - lv4 * extrusion, 1.0);
		EmitVertex();
		EndPrimitive();

/*

TODO:
- Project to infinity again
- Shrink caps proportional to dilation
- Fix gaps between extruded quads
- Add more geometry rounded?


*/


		//Normals for each adjacent face.
		//If the edge faces away from the light (dot < 0),
		//the shared edge is part of the mesh silhouette.
		normal = cross(e3,e1); //Normal of face 0,1,2
		vec3 dilation_offset = normalize(cross(e1, lv0)) * dilation;

		if (dot(normal, lv0) <= 0)
			quad(pos[0], dilation_offset+pos[0]-lv0*extrusion, pos[2], dilation_offset+pos[2]-lv2*extrusion);

		normal = cross(e4,e5); //Normal of face 2,3,4
		dilation_offset = normalize(cross(e5, lv2)) * dilation;

		if (dot(normal, lv2) <= 0)
			quad(pos[2], dilation_offset+pos[2]-lv2*extrusion, pos[4], dilation_offset+pos[4]-lv4*extrusion);

		normal = cross(e2,e6); //Normal of face 4,5,0
		dilation_offset = normalize(cross(e2, lv4)) * dilation;

		if (dot(normal, lv4) <= 0)
			quad(pos[4], dilation_offset+pos[4]-lv4*extrusion, pos[0], dilation_offset+pos[0]-lv0*extrusion);
	}
}]],


shadowf = --[[glsl]][[
		#version 330
		void main()
		{
		}]],


forwardvshadowed = --[[glsl]][[
	#version 330
	in vec3 position;
	layout(location=1) in vec3 normal;
	in vec3 color;

	uniform float log_depth_intermediate_factor;

	uniform mat4 model_matrix;
	uniform mat4 model_view_normal_matrix;
	uniform mat4 model_view_projection_matrix;
	uniform mat4 shadow_view_projection_matrix;
	uniform float hella_time;

	out vec3 fPos;
	out vec4 sPos;
	out vec3 fNormal;
	out vec3 fColor;

	void main()
	{
		gl_Position = model_view_projection_matrix * vec4(position, 1.0);
		//gl_Position.z = (log2(max(1e-6, 1.0 + gl_Position.z)) * log_depth_intermediate_factor - 1.0) * gl_Position.w;
		fPos = vec3(model_matrix * vec4(position, 1));
		sPos = shadow_view_projection_matrix * vec4(position, 1.0);

		fNormal = vec3(vec4(normal, 0.0) * model_view_normal_matrix);
		fColor = color;
	}]],


forwardfshadowed = --[[glsl]][[
	uniform vec3 uLight_pos; //Light position in world space.
	uniform vec3 uLight_col; //Light color.
	uniform vec4 uLight_attr; //Light attributes. Falloff factors, then intensity.
	uniform vec3 camera_position; //Camera position in world space.
	uniform sampler2D shadow_map;
	uniform bool ambient_pass = false;
	uniform int show_depth = 0;

	uniform vec3 override_col = vec3(1.0, 1.0, 1.0);

	#define CONSTANT    0
	#define LINEAR      1
	#define EXPONENTIAL 2
	#define INTENSITY   3
	#define M_PI 3.1415926535897932384626433832795

	in vec3 fPos;
	in vec4 sPos;
	in vec3 fNormal;
	in vec3 fColor;

	out vec4 LFragment;

	// set important material values
	float roughnessValue = 0.5; // 0 : smooth, 1: rough
	float F0 = 0.2; // fresnel reflectance at normal incidence
	float k = 0.2; // fraction of diffuse reflection (specular reflection = 1 - k)
	void point_light_fresnel(in vec3 l, in vec3 v, in vec3 normal, out float specular, out float diffuse);

	vec3 sky_color(vec3 v, vec3 s, vec3 c)
	{
		float sun = clamp(dot(v, s), 0.0, 1.0); //sun direction, determines brightness
		return mix(vec3(0.0), vec3(0.0, 0.0, 1.0)*length(c), sun) + c*pow(sun, 2);
	}

	void main() {
		float gamma = 2.2;
		vec3 color = fColor;
		//roughnessValue = pow(0.2*length(color), 8);
		color *= override_col;
		vec3 normal = normalize(fNormal);
		vec3 final_color = vec3(0.0);
		float specular, diffuse;
		vec3 diffuse_frag = vec3(0.0);
		vec3 specular_frag = vec3(0.0);
		vec3 v = normalize(camera_position - fPos); //View vector.
		if (ambient_pass) {
			vec3 l = normalize(uLight_pos-fPos);
			point_light_fresnel(l, v, normal, specular, diffuse);
			diffuse_frag = color*diffuse*sky_color(normal, normalize(vec3(0.1, 0.8, 0.1)), uLight_col);
			specular_frag = color*specular*sky_color(reflect(v, normal), normalize(vec3(0.1, 0.8, 0.1)), uLight_col);
			diffuse_frag = color*diffuse*uLight_col;
			specular_frag = color*specular*uLight_col;
		} else {
			float dist = distance(fPos, uLight_pos);
			float attenuation = uLight_attr[CONSTANT] + uLight_attr[LINEAR]*dist + uLight_attr[EXPONENTIAL]*dist*dist;
			vec3 l = normalize(uLight_pos-fPos); //Light vector.
			point_light_fresnel(l, v, normal, specular, diffuse);
			diffuse_frag = color*uLight_col*uLight_attr[INTENSITY]*diffuse/attenuation;
			specular_frag = color*uLight_col*uLight_attr[INTENSITY]*specular/attenuation;
		}

		vec3 shadowPos = sPos.xyz / sPos.w;
		vec2 shadow_coords = shadowPos.xy;
		float shadow_depth = texture(shadow_map, shadow_coords).r;
		final_color += (diffuse_frag + specular_frag);

		if (shadowPos.z > shadow_depth && shadow_depth < 1.0)
			final_color *= 0.5;

		/*
		if (shadowPos.z > 1.0)
			final_color.r *= 0.5;

		if (shadow_depth > 1.0)
			final_color.g *= 0.5;
		*/

		//vec3 fog_color = vec3(0.0, 0.0, 0.0);
		//final_color = mix(final_color, fog_color, pow(distance(fPos, camera_position)/1000, 4.0));

		//Tone mapping.
		//float x = 0.0001;
		//final_color = (final_color * (6.2 * x + 0.5))/(final_color * (6.2 * final_color + 1.7) + 0.06);
		final_color = final_color / (final_color + vec3(1.0));
		//final_color = final_color / (max(max(final_color.x, final_color.y), final_color.z) + 1);

		//Gamma correction.
		final_color = pow(final_color, vec3(1.0 / gamma));
		if (show_depth == 1)
			final_color = vec3(shadowPos.z * 0.5 + 0.5);
		else if (show_depth == 2)
			final_color = vec3(shadow_depth);
		LFragment = vec4(final_color, 1.0);
	}]],

	blurv = --[[glsl]][[
	#version 330
	in vec2 pos;
	in vec2 uv;
	out vec2 tex_coord;

	void main()
	{
		tex_coord = uv;
		gl_Position = vec4(pos, 0, 1);
	}]],

	blurf = --[[glsl]][[
	#version 330
	in vec2 position;
	in vec2 uv_coord;
	out vec4 LFragment;

	uniform sampler2D tex;
	uniform bool horizontal;

	const float weights[5] = float[5](0.227027,0.1945946,0.1216216,0.054054,0.0162162);

	void main()
	{
		vec2 tex_offset = 1.0 / textureSize(tex, 0); // gets size of single texel
		vec3 result = texture(tex, uv_coord).rgb * weights[0]; // current fragment's contribution
		if (horizontal) {
			for (int i = 1; i < 5; i++)
				result += texture(tex, uv_coord + vec2(tex_offset.x * float(i), 0.0)).rgb * weights[i];
		} else {
			for (int i = 1; i < 5; i++)
				result += texture(tex, uv_coord + vec2(0.0, tex_offset.y * float(i))).rgb * weights[i];
		}
		LFragment = vec4(result*1.1, 1);
	}
	]],

	terraingenv = --[[glsl]][[
	#version 330
	uniform int numrows;
	uniform int numcols;
	in float cell_index;
	out float fcell_index;

	]]..common.triangular_atlas..--[[glsl]][[

	void main()
	{
		gl_Position = vec4((atlas_position(cell_index, numrows, numcols) * 2.0 - 1.0), 0, 1.0);
		fcell_index = float(cell_index);
	}]],

	terraingenf = --[[glsl]][[
	#version 330

	in float fcell_index;
	out vec4 LFragment;

	void main()
	{
		//Select a random color from our array of colors, using the cell index
		vec3 colors[6] = vec3[6](
			vec3(0.2, 0.2, 0.2),
			vec3(0.4, 0.4, 0.4),
			vec3(0.6, 0.6, 0.6),
			vec3(0.8, 0.8, 0.8),
			vec3(1.0, 1.0, 1.0),
			vec3(1.0, 0.5, 0.5)
		);

		LFragment = vec4(colors[int(fcell_index) % 6], 1.0);
		//LFragment = vec4(1.0);
	}
	]],

	--TODO: Figure out what inputs I need for this, use it to render a basic heightmap into the tile
	planet_terrain_1f = --[[glsl]][[
	void main() {
		float timescale = 0.0;
		float light_dist = 50.0;
		vec3 light_pos = vec3(light_dist * sin(time*timescale), light_dist, light_dist * cos(time*timescale));

		vec2 uv = 2.0 * gl_FragCoord.xy / resolution - 1.0;
		uv.x *= resolution.x / resolution.y;

		vec3 ro = eye_pos;
		vec3 rd = normalize(dir_mat * vec3(uv, -focal_length));
		// vec3 rd = normalize(fposition.xyz - eye_pos);
		vec4 planet_atm = planet;
		planet_atm.w += atmosphere_height;

		//View ray intersection with planet surface sphere
		vec3 si = sphereIntersection(ro, rd, planet);
		//View ray intersection with atmosphere sphere
		vec3 sia = sphereIntersection(ro, rd, planet_atm);
		float b = sia.z;

		//Plane normal, cuts the planet into sun-facing and non-sun-facing
		vec3 pn = normalize(light_pos - planet.xyz);
		vec3 ci = cylinderIntersection(ro, rd, planet.xyz, pn, planet.w);
		//I need to consider six points:
		//0. Ray starts in atmosphere (same as 1)
		//1. Ray enters atmosphere
		//2. Ray enters shadow
		//3. Ray hits planet
		//4. Ray exits shadow
		//5. Ray exits atmosphere
		//These are used differently depending on the case.
		//IS = In-Scattering, OS = Out-Scattering
		//Viewed from space:
		//A. Ray enters atmosphere, exits atmosphere. Integrate IS/OS from 1 to 5.
			//1, 5,5,5
		//B. Ray enters atmosphere, hits planet (above shadow plane). Integrate IS/OS from 1 to 3.
			//1, 3,3,3
		//C. Ray enters atmosphere, hits shadow, then planet (below shadow plane). Integrate IS/OS from 1 to 2.
			//1, 2,2,2
		//D. Ray enters atmosphere, hits shadow, exits atmosphere, exits shadow. Integrate IS/OS from 1 to 2 (similar to C).
			//1, 2, 5,5
		//E. Ray enters atmosphere, hits shadow, exits shadow, exits atmosphere. Integrate IS/OS from 1 to 2, OS from 2 to 4, IS/OS from 4 to 5.
			//1, 2, 4, 5
		//F. Ray enters shadow, hits planet. No integration needed.
			//1,1,1,1 or 2,2,2,2 or 3,3,3,3
		//G. Ray enters shadow, enters atmosphere, exits shadow, then exits atmosphere. Integrate OS from 1 to 4, IS/OS from 4 to 5.
			//1,1, 4, 5
		//H. Ray enters shadow, exits shadow. No integration needed.
			//2,2,2,2
		//I. Ray enters shadow, enters atmosphere, exits atmosphere, exits shadow. No integration needed.
			//1,1,1,1
		//Viewed from within atmosphere:
		//J. Ray starts in sunlight, exits atmosphere. Integrate from 0 to 5.
			//0,0,0, 5
		//K. Ray starts in sunlight, enters shadow, exits atmosphere. Integrate from 0 to 2.
			//0, 2,2,2
		//L. Ray starts in sunlight, enters shadow, exits shadow, exits atmosphere. Integrate IS/OS from 0 to 2, OS from 2 to 4, IS/OS from 4 to 5.
			//0, 2, 4, 5
		//M. Ray starts in shadow, exits atmosphere. No integration needed.
			//0,0,0,0
		//N. Ray starts in shadow, exits shadow, exits atmosphere. Integrate OS from 0 to 4, IS/OS from 4 to 5.
			//0,0, 4, 5

		//This could be encoded as distances along the view ray rd.
		//A vec4 could hold up to three intervals, starting nearest the camera and moving away.
		//encode the IS/OS, OS, IS/OS intervals by putting their start and end points in the vector,
		//then integrate 3 times unconditionally. If an interval's end is the same as its start, the interval is 0-length and should be ignored.
		//(probably will end up being multiplied by 0 and will not affect the result)
		//x_y is IS/OS, y_z is OS, z_w is IS/OS. Any can be 0-length.

		//Random guess (think through the cases, see if it would work)
		//x. max(0, dist to first atmosphere intersection)
		//y. max(x, dist to first shadow intersection)
		//z. max(y, dist to second shadow intersection)
		//w. max(z, dist to second atmosphere intersection)
		//Works with: A(1115), 

		// vec4 intervals = vec4();

		// dot(a, b) where b is a unit vector is the scalar projection of a onto b.
		// If I want to have a plane centered at the planet origin to cap the shadow cylinder,
		// I can determine if a point lies above or below that plane by subtracting the planet center from it
		// and then dotting that vector with the plane normal.
		// This will give the point's distance (positive or negative) from that plane.
		// if (ci.x != -1.0) {
		// 	vec3 ca = ro+rd*ci.x;
		// 	float above = dot(pn, ca - planet.xyz);
		// 	if (above < 0.0)
		// 		b = max(ci.x, sia.y);
		// }

		vec3 color = vec3(0.0);
		vec2 samples = vec2(samples_ab, samples_cp);
		if (si.x >= 0) { //If view ray intersects planet, draw the surface
			vec3 p = ro + rd*si.y; //Planet surface position.
			NoisePosNormal nn = surfaceNoisePosNormal(normalize(p - planet.xyz));
			vec3 tint = mix(vec3(0.22, 0.32, 0.15), vec3(0.48, 0.45, 0.25), nn.noise.w); //Simple green-to-yellow transition with altitude
			vec3 params = vec3(0.8, 0.2, 0.2); //Roughness, F0 and k for my PBR lighting function
			float shoreval = -0.04; //Noise value for shoreline. Higher moves the oceans up.
			float shorediffuse = 0.0;
			if (nn.noise.w < shoreval) {
				nn.pos = (1.0 + 0.01 * nn.noise.w) * p;//p; //Ocean surface is constant-altitude
				nn.normal = normalize(p-planet.xyz); //Ocean surface is smooth
				float shoredist = distance(nn.noise.w, shoreval);
				tint = clamp(vec3(0.1, 0.2, 0.5) - shoredist / 5.0, 0.0, 1.0); //Darken ocean depths
				shorediffuse = pow(1.0 - shoredist, 13.0)/2.4; //Brighten shoreline
				params = vec3(0.22, 0.2, 0.1);
			}
			float specular, diffuse;
			point_light_fresnel_p(normalize(light_pos-nn.pos), normalize(eye_pos-nn.pos), nn.normal, params, specular, diffuse);
			color = specular + tint * diffuse + shorediffuse*pow(diffuse, 3.0);
			b = si.y;
			samples[0] /= 2.0; //Halve the number of AB samples over the planet surface, for performance
		}

		float planet_scale_factor = planet_scale / planet_atm.w;
		// float planet_scale_factor = 6371000.0 / (1.0 + 0.02);
		// float H = 8500.0 / (6371000.0 / (1.0 + 0.02));
		//float H = scale_height / planet_scale_factor;
		//scale height is 8500.0
		//planet_scale is 6371000.0
		//planet_radius is 1
		//atmosphere height is 0.02
		// if (uv.x+uv.y > 0.0) {
		// if () {
			color += atmosphere1(ro, rd, vec4(sia.y, b, 0.0, 0.0), 0.0, planet, planet_atm.w, light_pos, vec3(1.0), planet_scale_factor, samples);
			// color += vec3(.2,.0,.0);
		// } else {
			// color += atmosphere3(ro, rd, vec4(sia.y, b, 0.0, 0.0), (sia.z-sia.y)/2.0, planet, planet_atm.w, light_pos, vec3(1.0), planet_scale_factor, samples);
		// }

		//minka.com? clouds

		// float d = ((sia.z - sia.y) - (si.z - si.y));
		// float unscientificScatter = dot(pa - planet.xyz, normalize(light_pos - planet.xyz))*0.5 + 0.5;
		// unscientificScatter *= unscientificScatter;
		// color += vec3(0.5, 0.72, 0.99) * smoothstep(0.965, 1.0, dot(rd, normalize(planet.xyz-eye_pos))) * d * unscientificScatter;
		// if (sia.x < 0.0)
		// 	color = vec3(0.1);

		// float gamma = 2.2;
		// color = color / (color + vec3(1.0)); //Tone mapping.
		// color = pow(color, vec3(1.0 / gamma)); //Gamma correction.
		// float fExposure = p_and_p;
		float fExposure = 1.0;
		color = 1.0 - exp(-fExposure * color);
		LFragment = vec4(color, 1.0);
		// LFragment = vec4(ftx, 0.0, 1.0);
		// if (selected_primitive == gl_PrimitiveID)
		// 	LFragment = vec4(1.0, 1.0, 1.0, 1.0);
	}]],
}

return shaders