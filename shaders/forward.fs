#version 330

uniform vec3 uLight_pos; //Light position in world space.
uniform vec3 uLight_col; //Light color.
uniform vec4 uLight_attr; //Light attributes. Falloff factors, then intensity.
uniform vec3 camera_position; //Camera position in world space.
uniform int ambient_pass;

#define CONSTANT    0
#define LINEAR      1
#define EXPONENTIAL 2
#define INTENSITY   3
#define M_PI 3.1415926535897932384626433832795

in vec3 fPos;
in vec3 fColor;
in vec3 fNormal;

out vec4 LFragment;

void point_light_fragment(in vec3 l, in vec3 v, in vec3 normal, out float specular, out float diffuse)
{
	vec3 h = normalize(l + v); //Halfway vector.

	float base = 1 - dot(v, h);
	float exponential = pow(base, 5.0);
	float F0 = 0.5;
	float fresnel = exponential + F0 * (1.0 - exponential);

	specular = fresnel * pow(max(0.0, dot(h, normal)), 32.0);
	diffuse = max(0.0, dot(l, normal));
}

void point_light_fragment2(in vec3 l, in vec3 v, in vec3 normal, out float specular, out float diffuse)
{
	// set important material values
	float roughnessValue = 0.1; // 0 : smooth, 1: rough
	float F0 = 0.8; // fresnel reflectance at normal incidence
	float k = 0.2; // fraction of diffuse reflection (specular reflection = 1 - k)

	vec3 h = normalize(l + v); //Halfway vector.
	
	// do the lighting calculation for each fragment.
	float NdotL = max(dot(normal, l), 0.0);
	specular = 0.0;
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
		
		specular = (fresnel * geoAtt * roughness) / (NdotV * NdotL * M_PI);
	}
	
	specular = max(0.0, NdotL * (k + specular * (1.0 - k)));
	diffuse = max(0.0, dot(l, normal));
}

vec3 sky_color(vec3 v, vec3 s, vec3 c)
{
	float sun = clamp(dot(v, s), 0.0, 1.0); //sun direction, determines brightness
	return mix(vec3(0.0), vec3(0.0, 0.0, 1.0)*length(c), sun) + c*pow(sun, 2);
}


void main() {
	float gamma = 2.2;
	vec3 normal = normalize(fNormal);
	vec3 final_color = vec3(0.0);
	float specular, diffuse;
	vec3 diffuse_frag = vec3(0.0);
	vec3 specular_frag = vec3(0.0);
	vec3 v = normalize(camera_position - fPos); //View vector.
	if (ambient_pass == 1) {
		point_light_fragment2(uLight_pos, v, normal, specular, diffuse);
		diffuse_frag = fColor*diffuse*sky_color(normal, normalize(vec3(0.1, 0.8, 0.1)), uLight_col);
		specular_frag = fColor*specular*sky_color(reflect(v, normal), normalize(vec3(0.1, 0.8, 0.1)), uLight_col);
	} else {
		float dist = distance(fPos, uLight_pos);
		float attenuation = uLight_attr[CONSTANT] + uLight_attr[LINEAR]*dist + uLight_attr[EXPONENTIAL]*dist*dist;
		vec3 l = normalize(uLight_pos-fPos); //Light vector.
		point_light_fragment2(l, v, normal, specular, diffuse);
		diffuse_frag = fColor*uLight_col*uLight_attr[INTENSITY]*diffuse/attenuation;
		specular_frag = fColor*uLight_col*uLight_attr[INTENSITY]*specular/attenuation;
	}

	final_color += (diffuse_frag + specular_frag);

	//Tone mapping.
	float x = 0.0001;
	final_color = (final_color * (6.2 * x + 0.5))/(final_color * (6.2 * final_color + 1.7) + 0.06); 
	final_color = final_color / (final_color + vec3(1.0));
	//Gamma correction.
	final_color = pow(final_color, vec3(1.0 / gamma));
	LFragment = vec4(final_color, 1.0);
	//LFragment = vec4(normal, 1.0);
}