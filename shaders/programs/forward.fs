#version 330

#define NUM_LIGHTS 4

uniform vec3 uLight_pos[NUM_LIGHTS]; //Light position in camera space.
uniform vec3 uLight_col[NUM_LIGHTS]; //Light color.
uniform vec4 uLight_attr[NUM_LIGHTS]; //Light attributes. Falloff factors, then intensity.
uniform vec3 camera_position;

#define CONSTANT    0
#define LINEAR      1
#define EXPONENTIAL 2
#define INTENSITY   3

struct light_fragment {
	float specular;
	float diffuse;
	float visual;
};

in vec3 fPos;
in vec3 fColor;
in vec3 fNormal;

out vec4 LFragment;

light_fragment point_light_fragment(vec3 l, vec3 v, vec3 normal)
{
	light_fragment tmp;
	vec3 h = normalize(l + v); //Halfway vector.

	float base = 1 - dot(v, h);
	float exponential = pow(base, 5.0);
	float F0 = 0.5;
	float fresnel = exponential + F0 * (1.0 - exponential);

	tmp.specular = fresnel * pow(max(0.0, dot(h, normal)), 32.0);
	tmp.diffuse = max(0.0, dot(l, normal));
	return tmp;
}

light_fragment point_light_fragment2(vec3 l, vec3 v, vec3 normal)
{
	light_fragment tmp;
    // set important material values
    float roughnessValue = 0.09; // 0 : smooth, 1: rough
    float F0 = 0.8; // fresnel reflectance at normal incidence
    float k = 0.2; // fraction of diffuse reflection (specular reflection = 1 - k)

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
    
    tmp.specular = NdotL * (k + specular * (1.0 - k));
    //tmp.specular = (fresnel * attr[INTENSITY] * pow(max(0.0, dot(h, normal)), 32.0)) / attenuation;
	tmp.diffuse = max(0.0, dot(l, normal));
	//tmp.visual = dot(light_pos, frag_pos);
	return tmp;
}

void main() {
	float gamma = 2.2;
	vec3 normal = normalize(fNormal);
	vec3 final_color = vec3(0.0);
	vec3 ambient_color = vec3(1.0, 0.9, 0.9);
	float ambient_intensity = 0.5;
	vec3 v = normalize(camera_position-fPos); //View vector.
	for (int i = 0; i < NUM_LIGHTS; i++) {
		float distance = distance(fPos, uLight_pos[i]);
		float attenuation = uLight_attr[i][CONSTANT] + (uLight_attr[i][LINEAR]*distance) + (uLight_attr[i][EXPONENTIAL]*distance*distance);

		vec3 l = normalize(uLight_pos[i]-fPos); //Light vector.

		light_fragment p = point_light_fragment2(l, v, normal);
		vec3 diffuse_frag = fColor*uLight_col[i]*uLight_attr[i][INTENSITY]*p.diffuse/attenuation;
		vec3 specular_frag = fColor*uLight_col[i]*uLight_attr[i][INTENSITY]*p.specular/attenuation;
		final_color = final_color + (diffuse_frag + specular_frag);
	}

	light_fragment p = point_light_fragment2(normalize(vec3(0.1, 0.9, 0.2)), v, fNormal);
	final_color += (p.diffuse+p.specular)*fColor*ambient_color*ambient_intensity;

	//Tone mapping.
	final_color = final_color / (final_color + vec3(1.0));
	//Gamma correction.
	final_color = pow(final_color, vec3(1.0 / gamma));
   	LFragment = vec4(final_color, 1.0);
   	//LFragment = vec4(fColor, 1.0);
}