return {


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

softshadowg = --[[glsl]][[
#version 330 core
layout (triangles_adjacency) in;
layout (triangle_strip, max_vertices = 18) out;

uniform vec3 gLightPos;
uniform int zpass;
uniform float log_depth_intermediate_factor;

in vec3 gPos[6];
in vec3 gNormal[6];
uniform mat4 projection_view_matrix;

float EPSILON = 0.0001;

// Emit a quad using a triangle strip
//void EmitQuadLines(vec3 StartVertex, vec3 EndVertex, vec3 lvStart, vec3 lvEnd, vec3 lpStart, vec3 lpEnd)
void EmitQuadLines(vec3 lvStart, vec3 lvEnd, vec3 lpStart, vec3 lpEnd)
{
	gl_Position = projection_view_matrix * vec4(lpStart, 1.0);
	gl_Position.z = (log2(max(1e-6, 1.0 + gl_Position.z)) * log_depth_intermediate_factor - 1.0) * gl_Position.w;
	EmitVertex();
	gl_Position = projection_view_matrix * vec4(lpEnd, 1.0);
	gl_Position.z = (log2(max(1e-6, 1.0 + gl_Position.z)) * log_depth_intermediate_factor - 1.0) * gl_Position.w;
	EmitVertex();
	gl_Position = projection_view_matrix * vec4(lvStart, 0.0);
	gl_Position.z = (log2(max(1e-6, 1.0 + gl_Position.z)) * log_depth_intermediate_factor - 1.0) * gl_Position.w;
	EmitVertex();
	gl_Position = projection_view_matrix * vec4(lvEnd , 0.0);
	gl_Position.z = (log2(max(1e-6, 1.0 + gl_Position.z)) * log_depth_intermediate_factor - 1.0) * gl_Position.w;
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
	vec3 normal = cross(e1, e2); //The face normal.
	vec3 lv0 = gLightPos - gPos[0]; //From vertex 0 to the light
	vec3 lv2 = gLightPos - gPos[2]; //From vertex 2 to the light
	vec3 lv4 = gLightPos - gPos[4]; //From vertex 4 to the light
	vec3 lp0 = (gPos[0] - lv0 * EPSILON);
	vec3 lp2 = (gPos[2] - lv2 * EPSILON);
	vec3 lp4 = (gPos[4] - lv4 * EPSILON);
	if (dot(normal, lv0) > 0) {
		if (zpass == 0) {
			//Front cap
			gl_Position = projection_view_matrix * vec4(lp0, 1.0);
			gl_Position.z = (log2(max(1e-6, 1.0 + gl_Position.z)) * log_depth_intermediate_factor - 1.0) * gl_Position.w;
			EmitVertex();
			gl_Position = projection_view_matrix * vec4(lp4, 1.0);
			gl_Position.z = (log2(max(1e-6, 1.0 + gl_Position.z)) * log_depth_intermediate_factor - 1.0) * gl_Position.w;
			EmitVertex();
			gl_Position = projection_view_matrix * vec4(lp2, 1.0);
			gl_Position.z = (log2(max(1e-6, 1.0 + gl_Position.z)) * log_depth_intermediate_factor - 1.0) * gl_Position.w;
			EmitVertex();
			EndPrimitive();

			//Back cap
			gl_Position = projection_view_matrix * vec4(-lv0, 0.0);
			gl_Position.z = (log2(max(1e-6, 1.0 + gl_Position.z)) * log_depth_intermediate_factor - 1.0) * gl_Position.w;
			EmitVertex();
			gl_Position = projection_view_matrix * vec4(-lv2, 0.0);
			gl_Position.z = (log2(max(1e-6, 1.0 + gl_Position.z)) * log_depth_intermediate_factor - 1.0) * gl_Position.w;
			EmitVertex();
			gl_Position = projection_view_matrix * vec4(-lv4, 0.0);
			gl_Position.z = (log2(max(1e-6, 1.0 + gl_Position.z)) * log_depth_intermediate_factor - 1.0) * gl_Position.w;
			EmitVertex();
			EndPrimitive();
		}

		normal = cross(e3,e1);

		if (dot(normal, lv0) <= 0)
			EmitQuadLines(-lv0, -lv2, lp0, lp2);

		normal = cross(e4,e5);

		if (dot(normal, lv2) <= 0)
			EmitQuadLines(-lv2, -lv4, lp2, lp4);

		normal = cross(e2,e6);

		if (dot(normal, lv4) <= 0)
			EmitQuadLines(-lv4, -lv0, lp4, lp0);
	}
}]]


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


}