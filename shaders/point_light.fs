#version 140
//Position, color, and normal buffers, in camera space.
uniform sampler2D gPositionMap;
uniform sampler2D gColorMap;
uniform sampler2D gNormalMap;
uniform vec2 gScreenSize;
uniform vec3 uLight_pos; //Light position in camera space.
uniform vec3 uLight_col; //Light color.
uniform vec4 uLight_attr; //Light attributes. Falloff factors, then intensity.

#define CONSTANT    0
#define LINEAR      1
#define EXPONENTIAL 2
#define INTENSITY   3

out vec4 LFragment;

struct light_fragment {
	float specular;
	float diffuse;
};

light_fragment point_light_fragment(vec3 world_pos, vec3 normal, vec3 light_pos, vec4 attr)
{
	light_fragment tmp;
	float distance = distance(world_pos, light_pos);
	float attenuation = attr[CONSTANT] + (attr[LINEAR]*distance) + (attr[EXPONENTIAL]*distance*distance);
	tmp.diffuse = (attr[INTENSITY] * clamp(dot(normalize(light_pos-world_pos), normal), 0, 1)) / attenuation;

	vec3 h = normalize(normalize(-world_pos) + normalize(light_pos-world_pos));
	tmp.specular = (attr[INTENSITY] * pow(max(0.0, dot(h, normalize(normal))), 32.0)) / attenuation;
	//tmp.specular = 0;
	return tmp;
}

void main() {
	//Grab attribute info from deferred framebuffer.
	vec2 TexCoord = gl_FragCoord.xy / gScreenSize;
	vec3 WorldPos = texture(gPositionMap, TexCoord).xyz;
	vec3 Color = texture(gColorMap, TexCoord).xyz;
	vec3 Normal = texture(gNormalMap, TexCoord).xyz;

	//float light_vis = 1/distance(lighting_pass_position, uLight.position);
	vec4 attr = uLight_attr;
	vec3 light_col = uLight_col;
	//attr[INTENSITY] *= 3;
	//light_col = vec3(1.0, 0.4, 0.4);
	light_fragment p = point_light_fragment(WorldPos, Normal, uLight_pos, attr);
   	LFragment = vec4(Color*light_col, 1.0) * (p.diffuse + p.specular);
	//LFragment = vec4(Normal, 1.0);
	//LFragment = vec4(WorldPos, 1.0);
}