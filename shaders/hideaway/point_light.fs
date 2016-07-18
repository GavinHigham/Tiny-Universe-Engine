#version 330
//Position, color, and normal buffers, in camera space.
uniform sampler2D gPositionMap;
uniform sampler2D gColorMap;
uniform sampler2D gNormalMap;
uniform vec2 gScreenSize;
uniform vec3 camera_position;
uniform vec3 uLight_pos; //Light position in camera space.
uniform vec3 uLight_col; //Light color.
uniform vec4 uLight_attr; //Light attributes. Falloff factors, then intensity.

#define CONSTANT    0
#define LINEAR      1
#define EXPONENTIAL 2
#define INTENSITY   3

layout (location = 0) out vec3 diffuse_light; 
layout (location = 1) out vec3 specular_light; 

struct light_fragment {
	float specular;
	float diffuse;
	float visual;
};

light_fragment point_light_fragment(vec3 frag_pos, vec3 normal, vec3 light_pos, vec4 attr)
{
	light_fragment tmp;
	float distance = distance(frag_pos, light_pos);
	float attenuation = attr[CONSTANT] + (attr[LINEAR]*distance) + (attr[EXPONENTIAL]*distance*distance);

	vec3 l = normalize(light_pos-frag_pos);
	vec3 v = normalize(camera_position-frag_pos);
	vec3 h = normalize(l + v);

	float base = 1 - dot(v, h);
	float exponential = pow(base, 5.0);
	float F0 = 0.5;
	float fresnel = exponential + F0 * (1.0 - exponential);

	tmp.specular = (fresnel * attr[INTENSITY] * pow(max(0.0, dot(h, normalize(normal))), 32.0)) / attenuation;
	tmp.diffuse = (attr[INTENSITY] * max(0.0, dot(l, normal))) / attenuation;
	tmp.visual = dot(light_pos, frag_pos);
	return tmp;
}

light_fragment point_light_fragment_2(vec3 frag_pos, vec3 normal, vec3 light_pos, vec4 attr)
{
	light_fragment tmp;
	float distance = distance(frag_pos, light_pos);
	float attenuation = attr[CONSTANT] + (attr[LINEAR]*distance) + (attr[EXPONENTIAL]*distance*distance);
    // set important material values
    float roughnessValue = 0.3; // 0 : smooth, 1: rough
    float F0 = 0.8; // fresnel reflectance at normal incidence
    float k = 0.2; // fraction of diffuse reflection (specular reflection = 1 - k)

	vec3 l = normalize(light_pos-frag_pos); //Light vector.
	vec3 v = normalize(camera_position-frag_pos); //View vector.
	vec3 h = normalize(l + v); //Halfway vector.

    // do the lighting calculation for each fragment.
    float NdotL = max(dot(normal, l), 0.0);
    
    float specular = 0.0;
    if(NdotL > 0.0)
    {
        // calculate intermediary values
        float NdotH = max(dot(normal, h), 0.0); 
        float NdotV = max(dot(normal, v), 0.0); // note: this could also be NdotL, which is the same value
        float VdotH = max(dot(v, h), 0.0);
        float mSquared = roughnessValue * roughnessValue;
        
        // geometric attenuation
        float NH2 = 2.0 * NdotH;
        float g1 = (NH2 * NdotV) / VdotH;
        float g2 = (NH2 * NdotL) / VdotH;
        float geoAtt = min(1.0, min(g1, g2));
     
        // roughness (or: microfacet distribution function)
        // beckmann distribution function
        float r1 = 1.0 / ( 4.0 * mSquared * pow(NdotH, 4.0));
        float r2 = (NdotH * NdotH - 1.0) / (mSquared * NdotH * NdotH);
        float roughness = r1 * exp(r2);
        
        // fresnel
        // Schlick approximation
        float fresnel = pow(1.0 - VdotH, 5.0);
        fresnel *= (1.0 - F0);
        fresnel += F0;
        
        specular = (fresnel * geoAtt * roughness) / (NdotV * NdotL * 3.14);
    }
    
    tmp.specular = attr[INTENSITY] * NdotL * (k + specular * (1.0 - k)) / attenuation;
    //tmp.specular = (fresnel * attr[INTENSITY] * pow(max(0.0, dot(h, normal)), 32.0)) / attenuation;
	tmp.diffuse = 0;//(attr[INTENSITY] * max(0.0, dot(l, normal))) / attenuation;
	//tmp.visual = dot(light_pos, frag_pos);
	return tmp;
}

void main() {
	//Grab attribute info from deferred framebuffer.
	vec2 TexCoord = gl_FragCoord.xy / gScreenSize;
	vec3 WorldPos = texture(gPositionMap, TexCoord).xyz;
	vec3 Color = texture(gColorMap, TexCoord).xyz;
	vec3 Normal = texture(gNormalMap, TexCoord).xyz;

	vec4 attr = uLight_attr;
	vec3 light_col = uLight_col;
	light_fragment p = point_light_fragment(WorldPos, Normal, uLight_pos, attr);
	diffuse_light = Color*light_col*p.diffuse;
	specular_light = Color*light_col*p.specular;
}