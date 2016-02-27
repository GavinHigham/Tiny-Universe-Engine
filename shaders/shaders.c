//GENERATED FILE, CHANGES WILL BE LOST ON NEXT RUN OF MAKE.
#include <GL/glew.h>
#include "shaders.h"
const char *effects_vs_file_path[] = {"shaders/programs/effects.vs"};
const char *effects_fs_file_path[] = {"shaders/programs/effects.fs"};

const char *effects_vs_source[] = {
"#version 330\n"
"\n"
"in vec3 vPos;\n"
"//uniform mat4 model_view_matrix;\n"
"\n"
"void main()\n"
"{\n"
"	gl_Position = vec4(vPos, 1);\n"
"}\n"
};
const char *effects_fs_source[] = {
"#version 330\n"
"//Position, color, and normal buffers, in camera space.\n"
"uniform sampler2D diffuse_light;\n"
"uniform sampler2D specular_light;\n"
"uniform vec2 gScreenSize;\n"
"\n"
"out vec4 LFragment;\n"
"\n"
"void main() {\n"
"	float gamma = 2.2;\n"
"\n"
"	vec2 TexCoord = gl_FragCoord.xy / gScreenSize;\n"
"	vec3 diffuse_frag = texture(diffuse_light, TexCoord).xyz;\n"
"	vec3 specular_frag = texture(specular_light, TexCoord).xyz;\n"
"\n"
"	vec3 final_color = diffuse_frag + specular_frag;\n"
"	//Tone mapping.\n"
"	final_color = final_color / (final_color + vec3(1.0));\n"
"	//Gamma correction.\n"
"	final_color = pow(final_color, vec3(1.0 / gamma));\n"
"	//LFragment = vec4(vec3(pow(gl_FragCoord.z, 2)), 1.0);\n"
"   	LFragment = vec4(final_color, 1.0);\n"
"}\n"
};
const GLchar *effects_attribute_names[] = {"vNormal", "vColor", "vPos"};
const GLchar *effects_uniform_names[] = {"gPositionMap", "gNormalMap", "gColorMap", "model_matrix", "projection_matrix", "gScreenSize", "diffuse_light", "specular_light", "uLight_attr", "uLight_col", "uLight_pos", "model_view_matrix", "MVM", "model_view_normal_matrix", "transp_model_matrix", "camera_position"};
static const int effects_attribute_count = sizeof(effects_attribute_names)/sizeof(effects_attribute_names[0]);
static const int effects_uniform_count = sizeof(effects_uniform_names)/sizeof(effects_uniform_names[0]);
GLint effects_attributes[effects_attribute_count];
GLint effects_uniforms[effects_uniform_count];
struct shader_prog effects_program = {
	.handle = 0,
	.attr = {-1, -1, 0},
	.unif = {-1, -1, -1, -1, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1}
};
struct shader_info effects_info = {
	.vs_source = effects_vs_source,
	.fs_source = effects_fs_source,
	.gs_source = NULL,
	.attr_names = effects_attribute_names,
	.unif_names = effects_uniform_names,
	.vs_file_path = effects_vs_file_path,
	.fs_file_path = effects_fs_file_path,
	.gs_file_path = NULL
};
const char *point_light_vs_file_path[] = {"shaders/programs/point_light.vs"};
const char *point_light_fs_file_path[] = {"shaders/programs/point_light.fs"};

const char *point_light_vs_source[] = {
"#version 330\n"
"\n"
"uniform mat4 MVM;\n"
"uniform mat4 projection_matrix;\n"
"\n"
"in vec3 vPos;\n"
"void main() {\n"
"	gl_Position = projection_matrix * (MVM * vec4(vPos, 1));\n"
"}\n"
};
const char *point_light_fs_source[] = {
"#version 330\n"
"//Position, color, and normal buffers, in camera space.\n"
"uniform sampler2D gPositionMap;\n"
"uniform sampler2D gColorMap;\n"
"uniform sampler2D gNormalMap;\n"
"uniform vec2 gScreenSize;\n"
"uniform vec3 camera_position;\n"
"uniform vec3 uLight_pos; //Light position in camera space.\n"
"uniform vec3 uLight_col; //Light color.\n"
"uniform vec4 uLight_attr; //Light attributes. Falloff factors, then intensity.\n"
"\n"
"#define CONSTANT    0\n"
"#define LINEAR      1\n"
"#define EXPONENTIAL 2\n"
"#define INTENSITY   3\n"
"\n"
"layout (location = 0) out vec3 diffuse_light; \n"
"layout (location = 1) out vec3 specular_light; \n"
"\n"
"struct light_fragment {\n"
"	float specular;\n"
"	float diffuse;\n"
"	float visual;\n"
"};\n"
"\n"
"light_fragment point_light_fragment(vec3 frag_pos, vec3 normal, vec3 light_pos, vec4 attr)\n"
"{\n"
"	light_fragment tmp;\n"
"	float distance = distance(frag_pos, light_pos);\n"
"	float attenuation = attr[CONSTANT] + (attr[LINEAR]*distance) + (attr[EXPONENTIAL]*distance*distance);\n"
"\n"
"	vec3 l = normalize(light_pos-frag_pos);\n"
"	vec3 v = normalize(camera_position-frag_pos);\n"
"	vec3 h = normalize(l + v);\n"
"\n"
"	float base = 1 - dot(v, h);\n"
"	float exponential = pow(base, 5.0);\n"
"	float F0 = 0.5;\n"
"	float fresnel = exponential + F0 * (1.0 - exponential);\n"
"\n"
"	tmp.specular = (fresnel * attr[INTENSITY] * pow(max(0.0, dot(h, normalize(normal))), 32.0)) / attenuation;\n"
"	tmp.diffuse = (attr[INTENSITY] * max(0.0, dot(l, normal))) / attenuation;\n"
"	tmp.visual = dot(light_pos, frag_pos);\n"
"	return tmp;\n"
"}\n"
"\n"
"light_fragment point_light_fragment_2(vec3 frag_pos, vec3 normal, vec3 light_pos, vec4 attr)\n"
"{\n"
"	light_fragment tmp;\n"
"	float distance = distance(frag_pos, light_pos);\n"
"	float attenuation = attr[CONSTANT] + (attr[LINEAR]*distance) + (attr[EXPONENTIAL]*distance*distance);\n"
"    // set important material values\n"
"    float roughnessValue = 0.3; // 0 : smooth, 1: rough\n"
"    float F0 = 0.8; // fresnel reflectance at normal incidence\n"
"    float k = 0.2; // fraction of diffuse reflection (specular reflection = 1 - k)\n"
"\n"
"	vec3 l = normalize(light_pos-frag_pos); //Light vector.\n"
"	vec3 v = normalize(camera_position-frag_pos); //View vector.\n"
"	vec3 h = normalize(l + v); //Halfway vector.\n"
"\n"
"    // do the lighting calculation for each fragment.\n"
"    float NdotL = max(dot(normal, l), 0.0);\n"
"    \n"
"    float specular = 0.0;\n"
"    if(NdotL > 0.0)\n"
"    {\n"
"        // calculate intermediary values\n"
"        float NdotH = max(dot(normal, h), 0.0); \n"
"        float NdotV = max(dot(normal, v), 0.0); // note: this could also be NdotL, which is the same value\n"
"        float VdotH = max(dot(v, h), 0.0);\n"
"        float mSquared = roughnessValue * roughnessValue;\n"
"        \n"
"        // geometric attenuation\n"
"        float NH2 = 2.0 * NdotH;\n"
"        float g1 = (NH2 * NdotV) / VdotH;\n"
"        float g2 = (NH2 * NdotL) / VdotH;\n"
"        float geoAtt = min(1.0, min(g1, g2));\n"
"     \n"
"        // roughness (or: microfacet distribution function)\n"
"        // beckmann distribution function\n"
"        float r1 = 1.0 / ( 4.0 * mSquared * pow(NdotH, 4.0));\n"
"        float r2 = (NdotH * NdotH - 1.0) / (mSquared * NdotH * NdotH);\n"
"        float roughness = r1 * exp(r2);\n"
"        \n"
"        // fresnel\n"
"        // Schlick approximation\n"
"        float fresnel = pow(1.0 - VdotH, 5.0);\n"
"        fresnel *= (1.0 - F0);\n"
"        fresnel += F0;\n"
"        \n"
"        specular = (fresnel * geoAtt * roughness) / (NdotV * NdotL * 3.14);\n"
"    }\n"
"    \n"
"    tmp.specular = attr[INTENSITY] * NdotL * (k + specular * (1.0 - k)) / attenuation;\n"
"    //tmp.specular = (fresnel * attr[INTENSITY] * pow(max(0.0, dot(h, normal)), 32.0)) / attenuation;\n"
"	tmp.diffuse = 0;//(attr[INTENSITY] * max(0.0, dot(l, normal))) / attenuation;\n"
"	//tmp.visual = dot(light_pos, frag_pos);\n"
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
"	vec4 attr = uLight_attr;\n"
"	vec3 light_col = uLight_col;\n"
"	light_fragment p = point_light_fragment(WorldPos, Normal, uLight_pos, attr);\n"
"	diffuse_light = Color*light_col*p.diffuse;\n"
"	specular_light = Color*light_col*p.specular;\n"
"}\n"
};
const GLchar *point_light_attribute_names[] = {"vNormal", "vColor", "vPos"};
const GLchar *point_light_uniform_names[] = {"gPositionMap", "gNormalMap", "gColorMap", "model_matrix", "projection_matrix", "gScreenSize", "diffuse_light", "specular_light", "uLight_attr", "uLight_col", "uLight_pos", "model_view_matrix", "MVM", "model_view_normal_matrix", "transp_model_matrix", "camera_position"};
static const int point_light_attribute_count = sizeof(point_light_attribute_names)/sizeof(point_light_attribute_names[0]);
static const int point_light_uniform_count = sizeof(point_light_uniform_names)/sizeof(point_light_uniform_names[0]);
GLint point_light_attributes[point_light_attribute_count];
GLint point_light_uniforms[point_light_uniform_count];
struct shader_prog point_light_program = {
	.handle = 0,
	.attr = {-1, -1, 0},
	.unif = {0, 0, 0, -1, 0, 0, -1, -1, 0, 0, 0, -1, 0, -1, -1, 0}
};
struct shader_info point_light_info = {
	.vs_source = point_light_vs_source,
	.fs_source = point_light_fs_source,
	.gs_source = NULL,
	.attr_names = point_light_attribute_names,
	.unif_names = point_light_uniform_names,
	.vs_file_path = point_light_vs_file_path,
	.fs_file_path = point_light_fs_file_path,
	.gs_file_path = NULL
};
const char *deferred_vs_file_path[] = {"shaders/programs/deferred.vs"};
const char *deferred_fs_file_path[] = {"shaders/programs/deferred.fs"};

const char *deferred_vs_source[] = {
"#version 330 \n"
"\n"
"in vec3 vPos; \n"
"in vec3 vColor; \n"
"in vec3 vNormal; \n"
"\n"
"//uniform mat4 NMVM;\n"
"uniform mat4 model_view_matrix;\n"
"uniform mat4 projection_matrix;\n"
"uniform mat4 model_matrix;\n"
"uniform mat4 transp_model_matrix;\n"
"\n"
"out vec3 fPos;\n"
"out vec3 fObjectPos;\n"
"out vec3 fColor;\n"
"out vec3 fNormal;\n"
"\n"
"void main()\n"
"{\n"
"	//vec4 vertex_view_pos =  MVM * vec4(vPos, 1);\n"
"	//gl_Position = projection_matrix * vertex_view_pos;\n"
"	//Pass along position and normal in camera space.\n"
"	//fPos = vec3(vertex_view_pos);\n"
"	//fNormal = vec3(vec4(vNormal, 0.0) * NMVM);\n"
"	fColor = vColor;\n"
"	//Pass along object-space position.\n"
"	fObjectPos = vPos;\n"
"\n"
"	gl_Position = projection_matrix * (model_view_matrix * vec4(vPos, 1));\n"
"	fPos = vec3(model_matrix * vec4(vPos, 1));\n"
"	fNormal = vec3(vec4(vNormal, 0.0) * transp_model_matrix);\n"
"}\n"
};
const char *deferred_fs_source[] = {
"#version 330\n"
"\n"
"in vec3 fPos;\n"
"in vec3 fObjectPos;\n"
"in vec3 fColor;\n"
"in vec3 fNormal;\n"
"\n"
"layout (location = 0) out vec3 WorldPosOut; \n"
"layout (location = 1) out vec3 DiffuseOut; \n"
"layout (location = 2) out vec3 NormalOut;\n"
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
const GLchar *deferred_uniform_names[] = {"gPositionMap", "gNormalMap", "gColorMap", "model_matrix", "projection_matrix", "gScreenSize", "diffuse_light", "specular_light", "uLight_attr", "uLight_col", "uLight_pos", "model_view_matrix", "MVM", "model_view_normal_matrix", "transp_model_matrix", "camera_position"};
static const int deferred_attribute_count = sizeof(deferred_attribute_names)/sizeof(deferred_attribute_names[0]);
static const int deferred_uniform_count = sizeof(deferred_uniform_names)/sizeof(deferred_uniform_names[0]);
GLint deferred_attributes[deferred_attribute_count];
GLint deferred_uniforms[deferred_uniform_count];
struct shader_prog deferred_program = {
	.handle = 0,
	.attr = {0, 0, 0},
	.unif = {-1, -1, -1, 0, 0, -1, -1, -1, -1, -1, -1, 0, -1, -1, 0, -1}
};
struct shader_info deferred_info = {
	.vs_source = deferred_vs_source,
	.fs_source = deferred_fs_source,
	.gs_source = NULL,
	.attr_names = deferred_attribute_names,
	.unif_names = deferred_uniform_names,
	.vs_file_path = deferred_vs_file_path,
	.fs_file_path = deferred_fs_file_path,
	.gs_file_path = NULL
};
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
"	gl_Position = projection_matrix * (model_view_matrix * vec4(vPos, 1));\n"
"	fPos = vec3(model_matrix * vec4(vPos, 1));\n"
"}\n"
};
const char *skybox_fs_source[] = {
"#version 330\n"
"\n"
"uniform vec3 camera_position;\n"
"\n"
"in vec3 fPos;\n"
"out vec4 LFragment;\n"
"\n"
"void main() {\n"
"	vec3 sun_dir = normalize(vec3(1, 1, 1));\n"
"	float gamma = 2.2;\n"
"	vec3 v = normalize(camera_position-fPos); //View vector.\n"
"	float sky = dot(v, vec3(0, 1, 0)); //sky direction, determines whiteness\n"
"	float sun = dot(v, sun_dir); //sun direction, determines brightness\n"
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
const GLchar *skybox_uniform_names[] = {"gPositionMap", "gNormalMap", "gColorMap", "model_matrix", "projection_matrix", "gScreenSize", "diffuse_light", "specular_light", "uLight_attr", "uLight_col", "uLight_pos", "model_view_matrix", "MVM", "model_view_normal_matrix", "transp_model_matrix", "camera_position"};
static const int skybox_attribute_count = sizeof(skybox_attribute_names)/sizeof(skybox_attribute_names[0]);
static const int skybox_uniform_count = sizeof(skybox_uniform_names)/sizeof(skybox_uniform_names[0]);
GLint skybox_attributes[skybox_attribute_count];
GLint skybox_uniforms[skybox_uniform_count];
struct shader_prog skybox_program = {
	.handle = 0,
	.attr = {-1, -1, 0},
	.unif = {-1, -1, -1, 0, 0, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, 0}
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
const char *point_light_wireframe_vs_file_path[] = {"shaders/programs/point_light_wireframe.vs"};
const char *point_light_wireframe_fs_file_path[] = {"shaders/programs/point_light_wireframe.fs"};

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
const GLchar *point_light_wireframe_uniform_names[] = {"gPositionMap", "gNormalMap", "gColorMap", "model_matrix", "projection_matrix", "gScreenSize", "diffuse_light", "specular_light", "uLight_attr", "uLight_col", "uLight_pos", "model_view_matrix", "MVM", "model_view_normal_matrix", "transp_model_matrix", "camera_position"};
static const int point_light_wireframe_attribute_count = sizeof(point_light_wireframe_attribute_names)/sizeof(point_light_wireframe_attribute_names[0]);
static const int point_light_wireframe_uniform_count = sizeof(point_light_wireframe_uniform_names)/sizeof(point_light_wireframe_uniform_names[0]);
GLint point_light_wireframe_attributes[point_light_wireframe_attribute_count];
GLint point_light_wireframe_uniforms[point_light_wireframe_uniform_count];
struct shader_prog point_light_wireframe_program = {
	.handle = 0,
	.attr = {-1, -1, 0},
	.unif = {-1, -1, -1, -1, 0, -1, -1, -1, -1, 0, -1, -1, 0, -1, -1, -1}
};
struct shader_info point_light_wireframe_info = {
	.vs_source = point_light_wireframe_vs_source,
	.fs_source = point_light_wireframe_fs_source,
	.gs_source = NULL,
	.attr_names = point_light_wireframe_attribute_names,
	.unif_names = point_light_wireframe_uniform_names,
	.vs_file_path = point_light_wireframe_vs_file_path,
	.fs_file_path = point_light_wireframe_fs_file_path,
	.gs_file_path = NULL
};
const char *forward_vs_file_path[] = {"shaders/programs/forward.vs"};
const char *forward_fs_file_path[] = {"shaders/programs/forward.fs"};
const char *forward_gs_file_path[] = {"shaders/programs/forward.gs"};
const char *forward_vs_source[] = {
"#version 330 \n"
"\n"
"in vec3 vColor;\n"
"in vec3 vPos; \n"
"in vec3 vNormal; \n"
"\n"
"uniform mat4 model_matrix;\n"
"uniform mat4 model_view_matrix;\n"
"uniform mat4 model_view_normal_matrix;\n"
"uniform mat4 projection_matrix;\n"
"\n"
"out vec3 gPos;\n"
"out vec3 gColor;\n"
"out vec3 gNormal;\n"
"\n"
"void main()\n"
"{\n"
"	gl_Position = projection_matrix * (model_view_matrix * vec4(vPos, 1));\n"
"	gPos = vec3(model_matrix * vec4(vPos, 1));\n"
"	gColor = vColor;\n"
"	gNormal = vec3(vec4(vNormal, 0.0) * model_view_normal_matrix);\n"
"}\n"
};
const char *forward_fs_source[] = {
"#version 330\n"
"\n"
"#define NUM_LIGHTS 4\n"
"\n"
"uniform vec3 uLight_pos[NUM_LIGHTS]; //Light position in world space.\n"
"uniform vec3 uLight_col[NUM_LIGHTS]; //Light color.\n"
"uniform vec4 uLight_attr[NUM_LIGHTS]; //Light attributes. Falloff factors, then intensity.\n"
"uniform vec3 camera_position; //Camera position in world space.\n"
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
"	vec3 ambient_color = vec3(1.0, 0.9, 0.9);\n"
"	float ambient_intensity = 0.1;\n"
"	float specular, diffuse;\n"
"	vec3 v = normalize(camera_position - fPos); //View vector.\n"
"	for (int i = 0; i < NUM_LIGHTS; i++) {\n"
"		float dist = distance(fPos, uLight_pos[i]);\n"
"		float attenuation = uLight_attr[i][CONSTANT] + uLight_attr[i][LINEAR]*dist + uLight_attr[i][EXPONENTIAL]*dist*dist;\n"
"\n"
"		vec3 l = normalize(uLight_pos[i]-fPos); //Light vector.\n"
"\n"
"		point_light_fragment2(l, v, normal, specular, diffuse);\n"
"		vec3 diffuse_frag = fColor*uLight_col[i]*uLight_attr[i][INTENSITY]*diffuse/attenuation;\n"
"		vec3 specular_frag = fColor*uLight_col[i]*uLight_attr[i][INTENSITY]*specular/attenuation;\n"
"		final_color += (diffuse_frag + specular_frag);\n"
"	}\n"
"\n"
"	point_light_fragment2(normalize(vec3(0.1, 0.9, 0.2)), v, fNormal, specular, diffuse);\n"
"	final_color += (diffuse+specular)*fColor*ambient_color*ambient_intensity;\n"
"\n"
"	//Tone mapping.\n"
"	final_color = final_color / (final_color + vec3(1.0));\n"
"	//Gamma correction.\n"
"	final_color = pow(final_color, vec3(1.0 / gamma));\n"
"	LFragment = vec4(final_color, 1.0);\n"
"	//LFragment = vec4(normal, 1.0);\n"
"}\n"
};
const char *forward_gs_source[] = {
"#version 330 core\n"
"layout (triangles) in;\n"
"layout (triangle_strip, max_vertices = 3) out;\n"
"\n"
"in vec3 gPos[3];\n"
"in vec3 gColor[3];\n"
"in vec3 gNormal[3];\n"
"out vec3 fPos;\n"
"out vec3 fColor;\n"
"out vec3 fNormal;\n"
"\n"
"void main() {\n"
"	for (int i = 0; i < 3; i++) {\n"
"	    gl_Position = gl_in[i].gl_Position;\n"
"	    fPos = gPos[i];\n"
"	    fColor = gColor[i];\n"
"	    fNormal = gNormal[i];\n"
"	    EmitVertex();\n"
"	}\n"
"    \n"
"    EndPrimitive();\n"
"} \n"
};
const GLchar *forward_attribute_names[] = {"vNormal", "vColor", "vPos"};
const GLchar *forward_uniform_names[] = {"gPositionMap", "gNormalMap", "gColorMap", "model_matrix", "projection_matrix", "gScreenSize", "diffuse_light", "specular_light", "uLight_attr", "uLight_col", "uLight_pos", "model_view_matrix", "MVM", "model_view_normal_matrix", "transp_model_matrix", "camera_position"};
static const int forward_attribute_count = sizeof(forward_attribute_names)/sizeof(forward_attribute_names[0]);
static const int forward_uniform_count = sizeof(forward_uniform_names)/sizeof(forward_uniform_names[0]);
GLint forward_attributes[forward_attribute_count];
GLint forward_uniforms[forward_uniform_count];
struct shader_prog forward_program = {
	.handle = 0,
	.attr = {0, 0, 0},
	.unif = {-1, -1, -1, 0, 0, -1, -1, -1, 0, 0, 0, 0, -1, 0, -1, 0}
};
struct shader_info forward_info = {
	.vs_source = forward_vs_source,
	.fs_source = forward_fs_source,
	.gs_source = forward_gs_source,
	.attr_names = forward_attribute_names,
	.unif_names = forward_uniform_names,
	.vs_file_path = forward_vs_file_path,
	.fs_file_path = forward_fs_file_path,
	.gs_file_path = forward_gs_file_path
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
"\n"
"out vec3 fPos;\n"
"\n"
"void main()\n"
"{\n"
"	gl_Position = projection_matrix * (model_view_matrix * vec4(vPos, 1));\n"
"	fPos = vPos;\n"
"}\n"
};
const char *stars_fs_source[] = {
"#version 330\n"
"\n"
"uniform vec3 camera_position; //Camera position in world space.\n"
"\n"
"in vec3 fPos;\n"
"out vec4 LFragment;\n"
"\n"
"void main() {\n"
"	// float gamma = 2.2;\n"
"	//vec3 final_color = vec3(1.0)*dot(fPos-camera_position, );\n"
"\n"
"	// //Tone mapping.\n"
"	// final_color = final_color / (final_color + vec3(1.0));\n"
"	// //Gamma correction.\n"
"	// final_color = pow(final_color, vec3(1.0 / gamma));\n"
"	// LFragment = vec4(final_color, 1.0);\n"
"	// //LFragment = vec4(normal, 1.0);\n"
"	vec3 refCam = camera_position;\n"
"	float blueness = 1-distance(fPos, vec3(0.0))/1000;\n"
"	LFragment = vec4(pow(blueness, 4), blueness*blueness, blueness, 1.0);\n"
"}\n"
};
const GLchar *stars_attribute_names[] = {"vNormal", "vColor", "vPos"};
const GLchar *stars_uniform_names[] = {"gPositionMap", "gNormalMap", "gColorMap", "model_matrix", "projection_matrix", "gScreenSize", "diffuse_light", "specular_light", "uLight_attr", "uLight_col", "uLight_pos", "model_view_matrix", "MVM", "model_view_normal_matrix", "transp_model_matrix", "camera_position"};
static const int stars_attribute_count = sizeof(stars_attribute_names)/sizeof(stars_attribute_names[0]);
static const int stars_uniform_count = sizeof(stars_uniform_names)/sizeof(stars_uniform_names[0]);
GLint stars_attributes[stars_attribute_count];
GLint stars_uniforms[stars_uniform_count];
struct shader_prog stars_program = {
	.handle = 0,
	.attr = {-1, -1, 0},
	.unif = {-1, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, 0}
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
struct shader_prog *shader_programs[] = {&effects_program, &point_light_program, &deferred_program, &skybox_program, &point_light_wireframe_program, &forward_program, &stars_program};
struct shader_info *shader_infos[] = {&effects_info, &point_light_info, &deferred_info, &skybox_info, &point_light_wireframe_info, &forward_info, &stars_info};
