//GENERATED FILE, CHANGES WILL BE LOST ON NEXT RUN OF MAKE.
#include <GL/glew.h>
#include "shaders.h"
const char *skybox_vs_file_path[] = {"shaders/programs/skybox.vs"};
const char *skybox_fs_file_path[] = {"shaders/programs/skybox.fs"};

const char *skybox_vs_source[] = {
"#version 330 \n"
"\n"
"in vec3 vPos; \n"
"\n"
"uniform mat4 model_matrix;\n"
"uniform mat4 model_view_matrix;\n"
"uniform mat4 projection_matrix;\n"
"\n"
"out vec3 fPos;\n"
"\n"
"void main()\n"
"{\n"
"	fPos = vec3(model_matrix * vec4(vPos, 1));\n"
"	gl_Position = projection_matrix * (model_view_matrix * vec4(vPos, 1));\n"
"}\n"
};
const char *skybox_fs_source[] = {
"#version 330\n"
"\n"
"uniform vec3 camera_position;\n"
"uniform vec3 sun_direction;\n"
"\n"
"in vec3 fPos;\n"
"out vec4 LFragment;\n"
"\n"
"void main() {\n"
"	float gamma = 2.2;\n"
"	vec3 v = normalize(camera_position-fPos); //View vector.\n"
"	float sky = dot(v, vec3(0, 1, 0)); //sky direction, determines whiteness\n"
"	float sun = dot(v, sun_direction); //sun direction, determines brightness\n"
"	vec3 ambient = vec3(0.0, 0.05, 0.2); //minimum amount of light\n"
"	vec3 sky_color = sun*vec3(0.1, 0.1, 1) + ambient;\n"
"	//vec3 sky_color = 4*s*vec3(0.1,0.3,0.8)+ambient;//*vec3(.6)+vec3(0.1,0.3,0.8);\n"
"\n"
"	vec3 final_color = sky_color;\n"
"\n"
"	//Tone mapping.\n"
"	final_color = final_color / (final_color + vec3(1.0));\n"
"	//Gamma correction.\n"
"	final_color = pow(final_color, vec3(1.0 / gamma));\n"
"   	LFragment = vec4(final_color, 1.0);\n"
"   	//LFragment = vec4(fColor, 1.0);\n"
"}\n"
};
const GLchar *skybox_attribute_names[] = {"vNormal", "vColor", "vPos"};
const GLchar *skybox_uniform_names[] = {"zpass", "projection_view_matrix", "model_matrix", "projection_matrix", "uLight_attr", "uLight_col", "uLight_pos", "sun_direction", "model_view_matrix", "eye_pos", "gLightPos", "model_view_normal_matrix", "ambient_pass", "uOrigin", "camera_position"};
static const int skybox_attribute_count = sizeof(skybox_attribute_names)/sizeof(skybox_attribute_names[0]);
static const int skybox_uniform_count = sizeof(skybox_uniform_names)/sizeof(skybox_uniform_names[0]);
GLint skybox_attributes[skybox_attribute_count];
GLint skybox_uniforms[skybox_uniform_count];
struct shader_prog skybox_program = {
	.handle = 0,
	.attr = {-1, -1, 0},
	.unif = {-1, -1, 0, 0, -1, -1, -1, 0, 0, -1, -1, -1, -1, -1, 0}
};
struct shader_info skybox_info = {
	.vs_source = skybox_vs_source,
	.fs_source = skybox_fs_source,
	.gs_source = NULL,
	.attr_names = skybox_attribute_names,
	.unif_names = skybox_uniform_names,
	.vs_file_path = skybox_vs_file_path,
	.fs_file_path = skybox_fs_file_path,
	.gs_file_path = NULL
};
const char *outline_vs_file_path[] = {"shaders/programs/outline.vs"};
const char *outline_fs_file_path[] = {"shaders/programs/outline.fs"};
const char *outline_gs_file_path[] = {"shaders/programs/outline.gs"};
const char *outline_vs_source[] = {
"#version 330 \n"
"\n"
"in vec3 vPos;\n"
"\n"
"uniform mat4 model_matrix;\n"
"uniform mat4 model_view_matrix;\n"
"uniform mat4 projection_matrix;\n"
"\n"
"out vec3 gPos;\n"
"\n"
"void main()\n"
"{\n"
"	gl_Position = projection_matrix * (model_view_matrix * vec4(vPos, 1));\n"
"	gPos = vec3(model_matrix * vec4(vPos, 1));\n"
"}\n"
};
const char *outline_fs_source[] = {
"#version 330\n"
"\n"
"// out vec4 LFragment;\n"
"\n"
"void main() {\n"
"	// LFragment = vec4(1.0, 0.0, 0.0, 1.0);\n"
"}\n"
};
const char *outline_gs_source[] = {
"#version 330 core\n"
"layout (triangles_adjacency) in;\n"
"layout (line_strip, max_vertices = 6) out;\n"
"\n"
"uniform vec3 uOrigin;\n"
"\n"
"in vec3 gPos[6];\n"
"vec4 z_nudge = vec4(0, 0, 0.1, 0);\n"
"\n"
"void EmitSegment(int StartIndex, int EndIndex)\n"
"{\n"
"	gl_Position = gl_in[StartIndex].gl_Position + z_nudge;\n"
"	EmitVertex();\n"
"	gl_Position = gl_in[EndIndex].gl_Position + z_nudge;\n"
"	EmitVertex();\n"
"	EndPrimitive();\n"
"}\n"
"\n"
"void main() {\n"
"	vec3 e1 = gPos[2] - gPos[0];\n"
"	vec3 e2 = gPos[4] - gPos[0];\n"
"	vec3 e3 = gPos[1] - gPos[0];\n"
"	vec3 e4 = gPos[3] - gPos[2];\n"
"	vec3 e5 = gPos[4] - gPos[2];\n"
"	vec3 e6 = gPos[5] - gPos[0];\n"
"	vec3 normal = cross(e1, e2);\n"
"	vec3 LightDir = uOrigin - gPos[0];\n"
"	if (dot(normal, LightDir) >= 0) {\n"
"\n"
"		normal = cross(e3,e1);\n"
"\n"
"		if (dot(normal, LightDir) <= 0) {\n"
"			EmitSegment(0, 2);\n"
"		}\n"
"\n"
"		normal = cross(e4,e5);\n"
"		LightDir = uOrigin - gPos[2];\n"
"\n"
"		if (dot(normal, LightDir) <=0) {\n"
"			EmitSegment(2, 4);\n"
"		}\n"
"\n"
"		normal = cross(e2,e6);\n"
"		LightDir = uOrigin - gPos[4];\n"
"\n"
"		if (dot(normal, LightDir) <= 0) {\n"
"			EmitSegment(4, 0);\n"
"		}\n"
"	}\n"
"}\n"
};
const GLchar *outline_attribute_names[] = {"vNormal", "vColor", "vPos"};
const GLchar *outline_uniform_names[] = {"zpass", "projection_view_matrix", "model_matrix", "projection_matrix", "uLight_attr", "uLight_col", "uLight_pos", "sun_direction", "model_view_matrix", "eye_pos", "gLightPos", "model_view_normal_matrix", "ambient_pass", "uOrigin", "camera_position"};
static const int outline_attribute_count = sizeof(outline_attribute_names)/sizeof(outline_attribute_names[0]);
static const int outline_uniform_count = sizeof(outline_uniform_names)/sizeof(outline_uniform_names[0]);
GLint outline_attributes[outline_attribute_count];
GLint outline_uniforms[outline_uniform_count];
struct shader_prog outline_program = {
	.handle = 0,
	.attr = {-1, -1, 0},
	.unif = {-1, -1, 0, 0, -1, -1, -1, -1, 0, -1, -1, -1, -1, 0, -1}
};
struct shader_info outline_info = {
	.vs_source = outline_vs_source,
	.fs_source = outline_fs_source,
	.gs_source = outline_gs_source,
	.attr_names = outline_attribute_names,
	.unif_names = outline_uniform_names,
	.vs_file_path = outline_vs_file_path,
	.fs_file_path = outline_fs_file_path,
	.gs_file_path = outline_gs_file_path
};
const char *forward_vs_file_path[] = {"shaders/programs/forward.vs"};
const char *forward_fs_file_path[] = {"shaders/programs/forward.fs"};

const char *forward_vs_source[] = {
"#version 330 \n"
"\n"
"in vec3 vColor;\n"
"in vec3 vPos; \n"
"in vec3 vNormal; \n"
"\n"
"uniform mat4 model_matrix;\n"
"uniform mat4 model_view_normal_matrix;\n"
"uniform mat4 projection_matrix;\n"
"uniform mat4 projection_view_matrix;\n"
"uniform mat4 model_view_matrix;\n"
"\n"
"out vec3 fPos;\n"
"out vec3 fColor;\n"
"out vec3 fNormal;\n"
"\n"
"void main()\n"
"{\n"
"	mat4 ref = model_view_matrix;\n"
"	mat4 ref2 = projection_matrix;\n"
"	gl_Position = projection_view_matrix * (model_matrix * vec4(vPos, 1));\n"
"	fPos = vec3(model_matrix * vec4(vPos, 1));\n"
"	fColor = vColor;\n"
"	fNormal = vec3(vec4(vNormal, 0.0) * model_view_normal_matrix);\n"
"}\n"
};
const char *forward_fs_source[] = {
"#version 330\n"
"\n"
"uniform vec3 uLight_pos; //Light position in world space.\n"
"uniform vec3 uLight_col; //Light color.\n"
"uniform vec4 uLight_attr; //Light attributes. Falloff factors, then intensity.\n"
"uniform vec3 camera_position; //Camera position in world space.\n"
"uniform int ambient_pass;\n"
"\n"
"#define CONSTANT    0\n"
"#define LINEAR      1\n"
"#define EXPONENTIAL 2\n"
"#define INTENSITY   3\n"
"#define M_PI 3.1415926535897932384626433832795\n"
"\n"
"in vec3 fPos;\n"
"in vec3 fColor;\n"
"in vec3 fNormal;\n"
"\n"
"out vec4 LFragment;\n"
"\n"
"void point_light_fragment(in vec3 l, in vec3 v, in vec3 normal, out float specular, out float diffuse)\n"
"{\n"
"	vec3 h = normalize(l + v); //Halfway vector.\n"
"\n"
"	float base = 1 - dot(v, h);\n"
"	float exponential = pow(base, 5.0);\n"
"	float F0 = 0.5;\n"
"	float fresnel = exponential + F0 * (1.0 - exponential);\n"
"\n"
"	specular = fresnel * pow(max(0.0, dot(h, normal)), 32.0);\n"
"	diffuse = max(0.0, dot(l, normal));\n"
"}\n"
"\n"
"void point_light_fragment2(in vec3 l, in vec3 v, in vec3 normal, out float specular, out float diffuse)\n"
"{\n"
"	// set important material values\n"
"	float roughnessValue = 0.1; // 0 : smooth, 1: rough\n"
"	float F0 = 0.8; // fresnel reflectance at normal incidence\n"
"	float k = 0.2; // fraction of diffuse reflection (specular reflection = 1 - k)\n"
"\n"
"	vec3 h = normalize(l + v); //Halfway vector.\n"
"	\n"
"	// do the lighting calculation for each fragment.\n"
"	float NdotL = max(dot(normal, l), 0.0);\n"
"	specular = 0.0;\n"
"	if(NdotL > 0.0)\n"
"	{\n"
"		// calculate intermediary values\n"
"		float NdotH = max(dot(normal, h), 0.0); \n"
"		float NdotV = max(dot(normal, v), 0.0); // note: this could also be NdotL, which is the same value\n"
"		float VdotH = max(dot(v, h), 0.0);\n"
"		float mSquared = roughnessValue * roughnessValue;\n"
"		\n"
"		// geometric attenuation\n"
"		float NH2 = 2.0 * NdotH;\n"
"		float g1 = (NH2 * NdotV) / VdotH;\n"
"		float g2 = (NH2 * NdotL) / VdotH;\n"
"		float geoAtt = min(1.0, min(g1, g2));\n"
"	 \n"
"		// roughness (or: microfacet distribution function)\n"
"		// beckmann distribution function\n"
"		float r1 = 1.0 / ( 4.0 * mSquared * pow(NdotH, 4.0));\n"
"		float r2 = (NdotH * NdotH - 1.0) / (mSquared * NdotH * NdotH);\n"
"		float roughness = r1 * exp(r2);\n"
"		\n"
"		// fresnel\n"
"		// Schlick approximation\n"
"		float fresnel = pow(1.0 - VdotH, 5.0);\n"
"		fresnel *= (1.0 - F0);\n"
"		fresnel += F0;\n"
"		\n"
"		specular = (fresnel * geoAtt * roughness) / (NdotV * NdotL * M_PI);\n"
"	}\n"
"	\n"
"	specular = max(0.0, NdotL * (k + specular * (1.0 - k)));\n"
"	diffuse = max(0.0, dot(l, normal));\n"
"}\n"
"\n"
"void main() {\n"
"	float gamma = 2.2;\n"
"	vec3 normal = normalize(fNormal);\n"
"	vec3 final_color = vec3(0.0);\n"
"	float specular, diffuse;\n"
"	vec3 diffuse_frag = vec3(0.0);\n"
"	vec3 specular_frag = vec3(0.0);\n"
"	vec3 v = normalize(camera_position - fPos); //View vector.\n"
"	if (ambient_pass == 1) {\n"
"		point_light_fragment2(uLight_pos, v, normal, specular, diffuse);\n"
"		diffuse_frag = fColor*uLight_col*diffuse;\n"
"		specular_frag = fColor*uLight_col*specular;\n"
"	} else {\n"
"		float dist = distance(fPos, uLight_pos);\n"
"		float attenuation = uLight_attr[CONSTANT] + uLight_attr[LINEAR]*dist + uLight_attr[EXPONENTIAL]*dist*dist;\n"
"		vec3 l = normalize(uLight_pos-fPos); //Light vector.\n"
"		point_light_fragment2(l, v, normal, specular, diffuse);\n"
"		diffuse_frag = fColor*uLight_col*uLight_attr[INTENSITY]*diffuse/attenuation;\n"
"		specular_frag = fColor*uLight_col*uLight_attr[INTENSITY]*specular/attenuation;\n"
"	}\n"
"\n"
"	final_color += (diffuse_frag + specular_frag);\n"
"\n"
"	//Tone mapping.\n"
"	final_color = final_color / (final_color + vec3(1.0));\n"
"	//Gamma correction.\n"
"	final_color = pow(final_color, vec3(1.0 / gamma));\n"
"	LFragment = vec4(final_color, 1.0);\n"
"	//LFragment = vec4(normal, 1.0);\n"
"}\n"
};
const GLchar *forward_attribute_names[] = {"vNormal", "vColor", "vPos"};
const GLchar *forward_uniform_names[] = {"zpass", "projection_view_matrix", "model_matrix", "projection_matrix", "uLight_attr", "uLight_col", "uLight_pos", "sun_direction", "model_view_matrix", "eye_pos", "gLightPos", "model_view_normal_matrix", "ambient_pass", "uOrigin", "camera_position"};
static const int forward_attribute_count = sizeof(forward_attribute_names)/sizeof(forward_attribute_names[0]);
static const int forward_uniform_count = sizeof(forward_uniform_names)/sizeof(forward_uniform_names[0]);
GLint forward_attributes[forward_attribute_count];
GLint forward_uniforms[forward_uniform_count];
struct shader_prog forward_program = {
	.handle = 0,
	.attr = {0, 0, 0},
	.unif = {-1, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, 0, 0, -1, 0}
};
struct shader_info forward_info = {
	.vs_source = forward_vs_source,
	.fs_source = forward_fs_source,
	.gs_source = NULL,
	.attr_names = forward_attribute_names,
	.unif_names = forward_uniform_names,
	.vs_file_path = forward_vs_file_path,
	.fs_file_path = forward_fs_file_path,
	.gs_file_path = NULL
};
const char *shadow_vs_file_path[] = {"shaders/programs/shadow.vs"};
const char *shadow_fs_file_path[] = {"shaders/programs/shadow.fs"};
const char *shadow_gs_file_path[] = {"shaders/programs/shadow.gs"};
const char *shadow_vs_source[] = {
"#version 330 \n"
"\n"
"in vec3 vPos;\n"
"in vec3 vNormal;\n"
"uniform mat4 model_matrix;\n"
"out vec3 gPos;\n"
"out vec3 gNormal;\n"
"\n"
"void main()\n"
"{\n"
"	gPos = vec3(model_matrix * vec4(vPos, 1));\n"
"	gNormal = vNormal;\n"
"}\n"
};
const char *shadow_fs_source[] = {
"#version 330\n"
"\n"
"void main() {\n"
"}\n"
};
const char *shadow_gs_source[] = {
"#version 330 core\n"
"layout (triangles_adjacency) in;\n"
"layout (triangle_strip, max_vertices = 18) out;\n"
"\n"
"uniform vec3 gLightPos;\n"
"uniform int zpass;\n"
"\n"
"in vec3 gPos[6];\n"
"in vec3 gNormal[6];\n"
"uniform mat4 projection_view_matrix;\n"
"\n"
"float EPSILON = 0.0001;\n"
"\n"
"// Emit a quad using a triangle strip\n"
"//void EmitQuadLines(vec3 StartVertex, vec3 EndVertex, vec3 lvStart, vec3 lvEnd, vec3 lpStart, vec3 lpEnd)\n"
"void EmitQuadLines(vec3 lvStart, vec3 lvEnd, vec3 lpStart, vec3 lpEnd)\n"
"{\n"
"	gl_Position = projection_view_matrix * vec4(lpStart, 1.0);\n"
"	EmitVertex();\n"
"	gl_Position = projection_view_matrix * vec4(lpEnd, 1.0);\n"
"	EmitVertex();\n"
"	gl_Position = projection_view_matrix * vec4(lvStart, 0.0);\n"
"	EmitVertex();\n"
"	gl_Position = projection_view_matrix * vec4(lvEnd , 0.0);\n"
"	EmitVertex();\n"
"	EndPrimitive(); \n"
"}\n"
"\n"
"void main() {\n"
"	vec3 e1 = gPos[2] - gPos[0];\n"
"	vec3 e2 = gPos[4] - gPos[0];\n"
"	vec3 e3 = gPos[1] - gPos[0];\n"
"	vec3 e4 = gPos[3] - gPos[2];\n"
"	vec3 e5 = gPos[4] - gPos[2];\n"
"	vec3 e6 = gPos[5] - gPos[0];\n"
"	vec3 normal = cross(e1, e2); //The face normal.\n"
"	vec3 lv0 = gLightPos - gPos[0]; //From vertex 0 to the light\n"
"	vec3 lv2 = gLightPos - gPos[2]; //From vertex 2 to the light\n"
"	vec3 lv4 = gLightPos - gPos[4]; //From vertex 4 to the light\n"
"	vec3 lp0 = (gPos[0] - lv0 * EPSILON);\n"
"	vec3 lp2 = (gPos[2] - lv2 * EPSILON);\n"
"	vec3 lp4 = (gPos[4] - lv4 * EPSILON);\n"
"	if (dot(normal, lv0) > 0) {\n"
"		if (zpass == 0) {\n"
"			//Front cap\n"
"			gl_Position = projection_view_matrix * vec4(lp0, 1.0);\n"
"			EmitVertex();\n"
"			gl_Position = projection_view_matrix * vec4(lp4, 1.0);\n"
"			EmitVertex();\n"
"			gl_Position = projection_view_matrix * vec4(lp2, 1.0);\n"
"			EmitVertex();\n"
"			EndPrimitive();\n"
"\n"
"			//Back cap\n"
"			gl_Position = projection_view_matrix * vec4(-lv0, 0.0);\n"
"			EmitVertex();\n"
"			gl_Position = projection_view_matrix * vec4(-lv2, 0.0);\n"
"			EmitVertex();\n"
"			gl_Position = projection_view_matrix * vec4(-lv4, 0.0);\n"
"			EmitVertex();\n"
"			EndPrimitive();\n"
"		}\n"
"\n"
"		normal = cross(e3,e1);\n"
"\n"
"		if (dot(normal, lv0) <= 0)\n"
"			EmitQuadLines(-lv0, -lv2, lp0, lp2);\n"
"\n"
"		normal = cross(e4,e5);\n"
"\n"
"		if (dot(normal, lv2) <= 0)\n"
"			EmitQuadLines(-lv2, -lv4, lp2, lp4);\n"
"\n"
"		normal = cross(e2,e6);\n"
"\n"
"		if (dot(normal, lv4) <= 0)\n"
"			EmitQuadLines(-lv4, -lv0, lp4, lp0);\n"
"	}\n"
"}\n"
};
const GLchar *shadow_attribute_names[] = {"vNormal", "vColor", "vPos"};
const GLchar *shadow_uniform_names[] = {"zpass", "projection_view_matrix", "model_matrix", "projection_matrix", "uLight_attr", "uLight_col", "uLight_pos", "sun_direction", "model_view_matrix", "eye_pos", "gLightPos", "model_view_normal_matrix", "ambient_pass", "uOrigin", "camera_position"};
static const int shadow_attribute_count = sizeof(shadow_attribute_names)/sizeof(shadow_attribute_names[0]);
static const int shadow_uniform_count = sizeof(shadow_uniform_names)/sizeof(shadow_uniform_names[0]);
GLint shadow_attributes[shadow_attribute_count];
GLint shadow_uniforms[shadow_uniform_count];
struct shader_prog shadow_program = {
	.handle = 0,
	.attr = {0, -1, 0},
	.unif = {0, 0, 0, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1}
};
struct shader_info shadow_info = {
	.vs_source = shadow_vs_source,
	.fs_source = shadow_fs_source,
	.gs_source = shadow_gs_source,
	.attr_names = shadow_attribute_names,
	.unif_names = shadow_uniform_names,
	.vs_file_path = shadow_vs_file_path,
	.fs_file_path = shadow_fs_file_path,
	.gs_file_path = shadow_gs_file_path
};
const char *stars_vs_file_path[] = {"shaders/programs/stars.vs"};
const char *stars_fs_file_path[] = {"shaders/programs/stars.fs"};

const char *stars_vs_source[] = {
"#version 330 \n"
"\n"
"in vec3 vPos; \n"
"\n"
"uniform mat4 model_view_matrix;\n"
"uniform mat4 projection_matrix;\n"
"uniform vec3 eye_pos;\n"
"\n"
"out vec3 fPos;\n"
"\n"
"float mag(vec3 v)\n"
"{\n"
"	return sqrt(v.x*v.x + v.y*v.y + v.z*v.z);\n"
"}\n"
"\n"
"void main()\n"
"{\n"
"	vec3 pos_ship_relative = vPos - eye_pos;\n"
"	//float star_dist = distance(eye_position, vPos);\n"
"	float star_dist = distance(eye_pos, vPos);\n"
"	/*\n"
"	float s = 80;\n"
"	float p = mag(pos_ship_relative) / s;\n"
"	p = s * 1-(pow(1.001, -1*(p*p)));\n"
"	pos_ship_relative = p * normalize(pos_ship_relative) + ship_position;\n"
"	vec4 pos = vec4(pos_ship_relative, 1);\n"
"	*/\n"
"	vec4 pos = vec4(vPos, 1);\n"
"	gl_Position = projection_matrix * (model_view_matrix * pos);\n"
"	gl_PointSize = 1;//(1-(star_dist / 700)) + 2;\n"
"	fPos = pos.xyz;\n"
"}\n"
};
const char *stars_fs_source[] = {
"#version 330\n"
"\n"
"in vec3 fPos;\n"
"out vec4 LFragment;\n"
"\n"
"void main() {\n"
"	vec2 pd = gl_PointCoord - vec2(0.5);\n"
"	float x = sqrt(2*(pd.x*pd.x + pd.y*pd.y));\n"
"	vec3 color = vec3(1-x*1.1, 1-x*1.11, 1-x);\n"
"	float b = 1-distance(fPos, vec3(0.0))/1000;\n"
"	LFragment = vec4((vec3(pow(b+0.25, 7.5)-0.1, pow(b, 1.8), b+0.3)+vec3(0.1))*(1-x), 1.0);\n"
"	//LFragment = vec4(vec3(attenuation), 1.0);\n"
"}\n"
};
const GLchar *stars_attribute_names[] = {"vNormal", "vColor", "vPos"};
const GLchar *stars_uniform_names[] = {"zpass", "projection_view_matrix", "model_matrix", "projection_matrix", "uLight_attr", "uLight_col", "uLight_pos", "sun_direction", "model_view_matrix", "eye_pos", "gLightPos", "model_view_normal_matrix", "ambient_pass", "uOrigin", "camera_position"};
static const int stars_attribute_count = sizeof(stars_attribute_names)/sizeof(stars_attribute_names[0]);
static const int stars_uniform_count = sizeof(stars_uniform_names)/sizeof(stars_uniform_names[0]);
GLint stars_attributes[stars_attribute_count];
GLint stars_uniforms[stars_uniform_count];
struct shader_prog stars_program = {
	.handle = 0,
	.attr = {-1, -1, 0},
	.unif = {-1, -1, -1, 0, -1, -1, -1, -1, 0, 0, -1, -1, -1, -1, -1}
};
struct shader_info stars_info = {
	.vs_source = stars_vs_source,
	.fs_source = stars_fs_source,
	.gs_source = NULL,
	.attr_names = stars_attribute_names,
	.unif_names = stars_uniform_names,
	.vs_file_path = stars_vs_file_path,
	.fs_file_path = stars_fs_file_path,
	.gs_file_path = NULL
};
struct shader_prog *shader_programs[] = {&skybox_program, &outline_program, &forward_program, &shadow_program, &stars_program};
struct shader_info *shader_infos[] = {&skybox_info, &outline_info, &forward_info, &shadow_info, &stars_info};
