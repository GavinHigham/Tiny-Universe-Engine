-- utility --

float rand(vec2 co){
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

-- lighting --

#define M_PI 3.1415926535897932384626433832795

void point_light_phong(in vec3 l, in vec3 v, in vec3 normal, out float specular, out float diffuse)
{
	vec3 h = normalize(l + v); //Halfway vector.

	float base = 1 - dot(v, h);
	float exponential = pow(base, 5.0);
	float F0 = 0.5;
	float fresnel = exponential + F0 * (1.0 - exponential);

	specular = fresnel * pow(max(0.0, dot(h, normal)), 32.0);
	diffuse = max(0.0, dot(l, normal));
}

// set important material values
// float roughnessValue = 0.8; // 0 : smooth, 1: rough
// float F0 = 0.2; // fresnel reflectance at normal incidence
// float k = 0.2; // fraction of diffuse reflection (specular reflection = 1 - k)

//TODO(Gavin): Figure out what this sort of shading is also called.
void point_light_fresnel(in vec3 l, in vec3 v, in vec3 normal, out float specular, out float diffuse)
{
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

--