//GENERATED FILE, CHANGES WILL BE LOST ON NEXT RUN OF MAKE.
#include <GL/glew.h>
#include "shaders.h"
const char *point_light_vs_file_path[] = {"shaders/point_light.vs"};
const char *point_light_fs_file_path[] = {"shaders/point_light.fs"};
const char *point_light_vs_source[] = {
"#version 150\n"
"\n"
"uniform mat4 MVM;\n"
"uniform mat4 projection_matrix;\n"
"\n"
"in vec3 vPos;\n"
"void main() {\n"
"	vec4 vPos_camera_space = MVM * vec4(vPos, 1);\n"
"	gl_Position = projection_matrix * vPos_camera_space;\n"
"}\n"
};
const char *point_light_fs_source[] = {
"#version 140\n"
"//Position, color, and normal buffers, in camera space.\n"
"uniform sampler2D gPositionMap;\n"
"uniform sampler2D gColorMap;\n"
"uniform sampler2D gNormalMap;\n"
"uniform vec2 gScreenSize;\n"
"uniform vec3 uLight_pos; //Light position in camera space.\n"
"uniform vec3 uLight_col; //Light color.\n"
"uniform vec4 uLight_attr; //Light attributes. Falloff factors, then intensity.\n"
"\n"
"#define CONSTANT    0\n"
"#define LINEAR      1\n"
"#define EXPONENTIAL 2\n"
"#define INTENSITY   3\n"
"\n"
"out vec4 LFragment;\n"
"\n"
"struct light_fragment {\n"
"	float specular;\n"
"	float diffuse;\n"
"};\n"
"\n"
"light_fragment point_light_fragment(vec3 world_pos, vec3 normal, vec3 light_pos, vec4 attr)\n"
"{\n"
"	light_fragment tmp;\n"
"	float distance = distance(world_pos, light_pos);\n"
"	float attenuation = attr[CONSTANT] + (attr[LINEAR]*distance) + (attr[EXPONENTIAL]*distance*distance);\n"
"	tmp.diffuse = (attr[INTENSITY] * clamp(dot(normalize(light_pos-world_pos), normal), 0, 1)) / attenuation;\n"
"\n"
"	vec3 h = normalize(normalize(-world_pos) + normalize(light_pos-world_pos));\n"
"	tmp.specular = (attr[INTENSITY] * pow(max(0.0, dot(h, normalize(normal))), 32.0)) / attenuation;\n"
"	//tmp.specular = 0;\n"
"	return tmp;\n"
"}\n"
"\n"
"void main() {\n"
"	//Grab attribute info from deferred framebuffer.\n"
"	vec2 TexCoord = gl_FragCoord.xy / gScreenSize;\n"
"	vec3 WorldPos = texture(gPositionMap, TexCoord).xyz;\n"
"	vec3 Color = texture(gColorMap, TexCoord).xyz;\n"
"	vec3 Normal = texture(gNormalMap, TexCoord).xyz;\n"
"\n"
"	//float light_vis = 1/distance(lighting_pass_position, uLight.position);\n"
"	vec4 attr = uLight_attr;\n"
"	vec3 light_col = uLight_col;\n"
"	//attr[INTENSITY] *= 3;\n"
"	//light_col = vec3(1.0, 0.4, 0.4);\n"
"	light_fragment p = point_light_fragment(WorldPos, Normal, uLight_pos, attr);\n"
"   	LFragment = vec4(Color*light_col, 1.0) * (p.diffuse + p.specular);\n"
"   	//LFragment = vec4(Normal, 1.0);\n"
"}\n"
};
const GLchar *point_light_attribute_names[] = {"vNormal", "vColor", "vPos"};
const GLchar *point_light_uniform_names[] = {"gPositionMap", "gNormalMap", "gColorMap", "projection_matrix", "gScreenSize", "uLight_attr", "uLight_col", "uLight_pos", "NMVM", "MVM"};
static const int point_light_attribute_count = sizeof(point_light_attribute_names)/sizeof(point_light_attribute_names[0]);
static const int point_light_uniform_count = sizeof(point_light_uniform_names)/sizeof(point_light_uniform_names[0]);
GLint point_light_attributes[point_light_attribute_count];
GLint point_light_uniforms[point_light_uniform_count];
struct shader_prog point_light_program = {
	.handle = 0,
	.attr = {-1, -1, 0},
	.unif = {0, 0, 0, 0, 0, 0, 0, 0, -1, 0}
};
struct shader_info point_light_info = {
	.vs_source = point_light_vs_source,
	.fs_source = point_light_fs_source,
	.attr_names = point_light_attribute_names,
	.unif_names = point_light_uniform_names,
	.vs_file_path = point_light_vs_file_path,
	.fs_file_path = point_light_fs_file_path
};
const char *deferred_vs_file_path[] = {"shaders/deferred.vs"};
const char *deferred_fs_file_path[] = {"shaders/deferred.fs"};
const char *deferred_vs_source[] = {
"#version 330 \n"
"\n"
"in vec3 vPos; \n"
"in vec3 vColor; \n"
"in vec3 vNormal; \n"
"\n"
"uniform mat4 projection_matrix;\n"
"uniform mat4 MVM;\n"
"uniform mat4 NMVM;\n"
"\n"
"out vec3 fPos;\n"
"out vec3 fObjectPos;\n"
"out vec3 fColor;\n"
"out vec3 fNormal;\n"
"\n"
"void main()\n"
"{\n"
"	vec4 vertex_view_pos =  MVM * vec4(vPos, 1);\n"
"	gl_Position = projection_matrix * vertex_view_pos;\n"
"	//Pass along position and normal in camera space.\n"
"	fPos = vec3(vertex_view_pos);\n"
"	fNormal = vec3(vec4(vNormal, 0.0) * NMVM);\n"
"	//Pass along color.\n"
"	fColor = vColor;\n"
"	//Pass along object-space position.\n"
"	fObjectPos = vPos;\n"
"}\n"
};
const char *deferred_fs_source[] = {
"#version 330\n"
"\n"
"in vec3 fPos;\n"
"in vec3 fObjectPos;\n"
"in vec3 fColor;\n"
"in vec3 fNormal; \n"
"\n"
"layout (location = 0) out vec3 WorldPosOut; \n"
"layout (location = 1) out vec3 DiffuseOut; \n"
"layout (location = 2) out vec3 NormalOut;\n"
"\n"
"//\n"
"// Description : Array and textureless GLSL 2D/3D/4D simplex \n"
"//               noise functions.\n"
"//      Author : Ian McEwan, Ashima Arts.\n"
"//  Maintainer : ijm\n"
"//     Lastmod : 20110822 (ijm)\n"
"//     License : Copyright (C) 2011 Ashima Arts. All rights reserved.\n"
"//               Distributed under the MIT License. See LICENSE file.\n"
"//               https://github.com/ashima/webgl-noise\n"
"// \n"
"\n"
"vec3 mod289(vec3 x) {\n"
"  return x - floor(x * (1.0 / 289.0)) * 289.0;\n"
"}\n"
"\n"
"vec4 mod289(vec4 x) {\n"
"  return x - floor(x * (1.0 / 289.0)) * 289.0;\n"
"}\n"
"\n"
"vec4 permute(vec4 x) {\n"
"     return mod289(((x*34.0)+1.0)*x);\n"
"}\n"
"\n"
"vec4 taylorInvSqrt(vec4 r)\n"
"{\n"
"  return 1.79284291400159 - 0.85373472095314 * r;\n"
"}\n"
"\n"
"float snoise(vec3 v)\n"
"  { \n"
"  const vec2  C = vec2(1.0/6.0, 1.0/3.0) ;\n"
"  const vec4  D = vec4(0.0, 0.5, 1.0, 2.0);\n"
"\n"
"// First corner\n"
"  vec3 i  = floor(v + dot(v, C.yyy) );\n"
"  vec3 x0 =   v - i + dot(i, C.xxx) ;\n"
"\n"
"// Other corners\n"
"  vec3 g = step(x0.yzx, x0.xyz);\n"
"  vec3 l = 1.0 - g;\n"
"  vec3 i1 = min( g.xyz, l.zxy );\n"
"  vec3 i2 = max( g.xyz, l.zxy );\n"
"\n"
"  //   x0 = x0 - 0.0 + 0.0 * C.xxx;\n"
"  //   x1 = x0 - i1  + 1.0 * C.xxx;\n"
"  //   x2 = x0 - i2  + 2.0 * C.xxx;\n"
"  //   x3 = x0 - 1.0 + 3.0 * C.xxx;\n"
"  vec3 x1 = x0 - i1 + C.xxx;\n"
"  vec3 x2 = x0 - i2 + C.yyy; // 2.0*C.x = 1/3 = C.y\n"
"  vec3 x3 = x0 - D.yyy;      // -1.0+3.0*C.x = -0.5 = -D.y\n"
"\n"
"// Permutations\n"
"  i = mod289(i); \n"
"  vec4 p = permute( permute( permute( \n"
"             i.z + vec4(0.0, i1.z, i2.z, 1.0 ))\n"
"           + i.y + vec4(0.0, i1.y, i2.y, 1.0 )) \n"
"           + i.x + vec4(0.0, i1.x, i2.x, 1.0 ));\n"
"\n"
"// Gradients: 7x7 points over a square, mapped onto an octahedron.\n"
"// The ring size 17*17 = 289 is close to a multiple of 49 (49*6 = 294)\n"
"  float n_ = 0.142857142857; // 1.0/7.0\n"
"  vec3  ns = n_ * D.wyz - D.xzx;\n"
"\n"
"  vec4 j = p - 49.0 * floor(p * ns.z * ns.z);  //  mod(p,7*7)\n"
"\n"
"  vec4 x_ = floor(j * ns.z);\n"
"  vec4 y_ = floor(j - 7.0 * x_ );    // mod(j,N)\n"
"\n"
"  vec4 x = x_ *ns.x + ns.yyyy;\n"
"  vec4 y = y_ *ns.x + ns.yyyy;\n"
"  vec4 h = 1.0 - abs(x) - abs(y);\n"
"\n"
"  vec4 b0 = vec4( x.xy, y.xy );\n"
"  vec4 b1 = vec4( x.zw, y.zw );\n"
"\n"
"  //vec4 s0 = vec4(lessThan(b0,0.0))*2.0 - 1.0;\n"
"  //vec4 s1 = vec4(lessThan(b1,0.0))*2.0 - 1.0;\n"
"  vec4 s0 = floor(b0)*2.0 + 1.0;\n"
"  vec4 s1 = floor(b1)*2.0 + 1.0;\n"
"  vec4 sh = -step(h, vec4(0.0));\n"
"\n"
"  vec4 a0 = b0.xzyw + s0.xzyw*sh.xxyy ;\n"
"  vec4 a1 = b1.xzyw + s1.xzyw*sh.zzww ;\n"
"\n"
"  vec3 p0 = vec3(a0.xy,h.x);\n"
"  vec3 p1 = vec3(a0.zw,h.y);\n"
"  vec3 p2 = vec3(a1.xy,h.z);\n"
"  vec3 p3 = vec3(a1.zw,h.w);\n"
"\n"
"//Normalise gradients\n"
"  vec4 norm = taylorInvSqrt(vec4(dot(p0,p0), dot(p1,p1), dot(p2, p2), dot(p3,p3)));\n"
"  p0 *= norm.x;\n"
"  p1 *= norm.y;\n"
"  p2 *= norm.z;\n"
"  p3 *= norm.w;\n"
"\n"
"// Mix final noise value\n"
"  vec4 m = max(0.6 - vec4(dot(x0,x0), dot(x1,x1), dot(x2,x2), dot(x3,x3)), 0.0);\n"
"  m = m * m;\n"
"  return 42.0 * dot( m*m, vec4( dot(p0,x0), dot(p1,x1), \n"
"                                dot(p2,x2), dot(p3,x3) ) );\n"
"  }\n"
"\n"
"void main() \n"
"{ \n"
"	WorldPosOut = fPos;\n"
"	vec3 noise_coords = fObjectPos*4;\n"
"	float noise = 0;\n"
"	// int num_iterations = 5;\n"
"	// for (int i = 0; i < num_iterations; i++) {\n"
"	// 	noise = noise + snoise(noise_coords);\n"
"	// 	noise_coords *= 2;\n"
"	// }\n"
"	// noise = (noise + 2)/(3 + num_iterations);\n"
"	DiffuseOut = fColor + noise;\n"
"	NormalOut = normalize(fNormal); \n"
"}\n"
};
const GLchar *deferred_attribute_names[] = {"vNormal", "vColor", "vPos"};
const GLchar *deferred_uniform_names[] = {"gPositionMap", "gNormalMap", "gColorMap", "projection_matrix", "gScreenSize", "uLight_attr", "uLight_col", "uLight_pos", "NMVM", "MVM"};
static const int deferred_attribute_count = sizeof(deferred_attribute_names)/sizeof(deferred_attribute_names[0]);
static const int deferred_uniform_count = sizeof(deferred_uniform_names)/sizeof(deferred_uniform_names[0]);
GLint deferred_attributes[deferred_attribute_count];
GLint deferred_uniforms[deferred_uniform_count];
struct shader_prog deferred_program = {
	.handle = 0,
	.attr = {0, 0, 0},
	.unif = {-1, -1, -1, 0, -1, -1, -1, -1, 0, 0}
};
struct shader_info deferred_info = {
	.vs_source = deferred_vs_source,
	.fs_source = deferred_fs_source,
	.attr_names = deferred_attribute_names,
	.unif_names = deferred_uniform_names,
	.vs_file_path = deferred_vs_file_path,
	.fs_file_path = deferred_fs_file_path
};
const char *point_light_wireframe_vs_file_path[] = {"shaders/point_light_wireframe.vs"};
const char *point_light_wireframe_fs_file_path[] = {"shaders/point_light_wireframe.fs"};
const char *point_light_wireframe_vs_source[] = {
"#version 150\n"
"\n"
"uniform mat4 MVM;\n"
"uniform mat4 projection_matrix;\n"
"\n"
"in vec3 vPos;\n"
"void main() {\n"
"	vec4 vPos_camera_space = MVM * vec4(vPos, 1);\n"
"	gl_Position = projection_matrix * vPos_camera_space;\n"
"}\n"
};
const char *point_light_wireframe_fs_source[] = {
"#version 140\n"
"uniform vec3 uLight_col; //Light color.\n"
"out vec4 LFragment;\n"
"\n"
"void main() {\n"
"   	LFragment = vec4(uLight_col, 1);\n"
"}\n"
};
const GLchar *point_light_wireframe_attribute_names[] = {"vNormal", "vColor", "vPos"};
const GLchar *point_light_wireframe_uniform_names[] = {"gPositionMap", "gNormalMap", "gColorMap", "projection_matrix", "gScreenSize", "uLight_attr", "uLight_col", "uLight_pos", "NMVM", "MVM"};
static const int point_light_wireframe_attribute_count = sizeof(point_light_wireframe_attribute_names)/sizeof(point_light_wireframe_attribute_names[0]);
static const int point_light_wireframe_uniform_count = sizeof(point_light_wireframe_uniform_names)/sizeof(point_light_wireframe_uniform_names[0]);
GLint point_light_wireframe_attributes[point_light_wireframe_attribute_count];
GLint point_light_wireframe_uniforms[point_light_wireframe_uniform_count];
struct shader_prog point_light_wireframe_program = {
	.handle = 0,
	.attr = {-1, -1, 0},
	.unif = {-1, -1, -1, 0, -1, -1, 0, -1, -1, 0}
};
struct shader_info point_light_wireframe_info = {
	.vs_source = point_light_wireframe_vs_source,
	.fs_source = point_light_wireframe_fs_source,
	.attr_names = point_light_wireframe_attribute_names,
	.unif_names = point_light_wireframe_uniform_names,
	.vs_file_path = point_light_wireframe_vs_file_path,
	.fs_file_path = point_light_wireframe_fs_file_path
};
struct shader_prog *shader_programs[] = {&point_light_program, &deferred_program, &point_light_wireframe_program};
struct shader_info *shader_infos[] = {&point_light_info, &deferred_info, &point_light_wireframe_info};
