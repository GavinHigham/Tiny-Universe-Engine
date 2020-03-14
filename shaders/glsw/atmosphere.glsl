-- vertex.GL33 --

in vec3 position; 
in vec2 tx;

uniform mat4 model_matrix;
uniform mat4 model_view_projection_matrix;
uniform float ico_scale;

out vec4 fposition;
out vec2 ftx;

void main()
{
	gl_Position = model_view_projection_matrix * vec4(position*ico_scale, 1.0);
	fposition = model_matrix * vec4(position, 1); //World-space surface position
	ftx = tx;
}

-- fragment.GL33 --

uniform mat3 dir_mat;
uniform vec3 eye_pos;
uniform vec4 planet; //Position, radius
uniform float atmosphere_height = 0.1;
uniform float focal_length;
uniform float time;
uniform float samples_ab;
uniform float samples_cp;
uniform float p_and_p; //"Poke and Prod", a variable I stick in as a factor when I want to tweak a value with my sliders to see what happens.
uniform float scale_height = 8500;
uniform float planet_scale = 6371000;
uniform vec2 resolution;
uniform int octaves = 5;
uniform vec3 air_b = vec3(0.00000519673, 0.0000121427, 0.0000296453);

in vec4 fposition;
in vec2 ftx;

out vec4 LFragment;

const highp float PI = 3.1415926535;
const highp float n = 1.00029;
const highp float N = 2.504e+25;
const highp float beta_factor = 8*pow(PI, 3.0)*pow(n*n-1.0, 2.0)/(3*N); //Divide by lambda^4 to get beta(lambda, 0)
highp vec3 air_lambda = air_b;

//Returns discriminant in x, lambda1 in y, lambda2 in z.
vec3 sphereIntersection(vec3 p, vec3 d, vec4 s);
//Simplex noise with gradient
vec4 snoise_grad(vec3 v);
//Google's Turbo color map function
vec3 TurboColormap(in float x);

// params.xyz:
//  x is roughnessValue = 0.8; // 0 : smooth, 1: rough
//  y is F0 = 0.2; // fresnel reflectance at normal incidence
//  z is k = 0.2; // fraction of diffuse reflection (specular reflection = 1 - k)
void point_light_fresnel_p(in vec3 l, in vec3 v, in vec3 normal, in vec3 params, out float specular, out float diffuse);

struct NoisePosNormal {
	vec4 noise;
	vec3 pos;
	vec3 normal;
};

//Value in w, grad in xyz
vec4 fbm(in vec3 seed)
{
	vec4 ret_val = vec4(0);
	float w = 0.5;
	for (int i = 0; i < octaves; i++) {
		float s = pow(2, i);
		float w = pow(w, i);
		ret_val += w * snoise_grad(s*seed);
	}
	return ret_val;
}

//Assuming x is a point on the surface of a sphere at the origin, return the noise value (with gradient), displaced position, and surface normal
NoisePosNormal surfaceNoisePosNormal(vec3 x)
{
	vec4 val = fbm(x*2.14); //2.14 is just a tweak factor to scale my continents to an appealing size
	vec3 g = val.xyz;
	float R = 1.0;
	float s = 0.01;
	vec3 p = (R + s * val.w) * x;
	vec3 h = g - dot(g, x)*x;
	vec3 n = x - s * h;
	NoisePosNormal nn = NoisePosNormal(val, p, n);
	return nn;
}

vec3 beta(vec3 lambda)
{
	return vec3(beta_factor) / pow(lambda, vec3(4.0));
}

//My gamma function takes cos(theta) instead of theta, to avoid needlessly doing cos(acos(cos_theta)),
//since cos(theta) is readily available as dot(v1, v2) where v1 and v2 are normalized vectors (with theta as the angle between them).
float gamma(float cos_theta)
{
	return 3.0*(1+cos_theta*cos_theta)/(16.0*PI);
}

//Atmosphere intersection nearest camera, other atmosphere intersection, planet, light position, light intensity
vec3 atmosphere(vec3 A, vec3 B, vec4 pl, float rA, vec3 l, vec3 lI, float planet_scale_factor, vec2 samples)
{
	//Number of sample points P we consider along AB
	float abSamples = samples[0];
	//Number of sample points Q we consider along CP
	float cpSamples = samples[1];
	//"scale height" of Earth for Rayleigh scattering
	float H = scale_height / planet_scale_factor;
	//Fraction of energy lost to scattering after one particle collision at sea level.
	vec3 b = air_lambda * planet_scale_factor;//beta(air_lambda);
	//Length of a sample segment on AB
	float d_ab = distance(A, B) / abSamples;
	//Sum of optical depth along AB
	float abSum = 0.0;
	vec3 T = vec3(0.0);
	for (int i = 0; i < abSamples; i++) {
		//Sample point P along the view ray
		vec3 P = mix(A, B, (float(i)+0.5)/abSamples);
		//Altitude of P
		float hP = max(distance(pl.xyz, P) - pl.w, 0.0);
		//If we dip into the planet, our ray ends there.
		float odP = exp(-hP/H) * d_ab;
		if (hP <= 0.0) {
			//TODO(Gavin): Use the planet intersection information to have the final step march a smaller distance.
			continue;
		}
		abSum += odP;

		//Find intersections of sunlight ray with atmosphere sphere
		vec3 ro = l;
		vec3 rd = normalize(P-l);
		vec3 si = sphereIntersection(ro, rd, vec4(pl.xyz, pl.w+rA));
		//Point C, where a ray of light travelling towards P from the sun first strikes the atmosphere
		vec3 C = ro + rd*si.y;
		//Length of a sample segment on CP
		float d_cp = distance(P, C)/cpSamples;
		//Sum of optical depth along CP
		float cpSum = 0.0;
		for (int j = 0; j < cpSamples; j++) {
			//Sample point Q along CP
			vec3 Q = mix(P, C, (float(j)+0.5)/cpSamples);
			//Altitude of Q
			float hQ = max(distance(pl.xyz, Q) - pl.w, 0.0);

			cpSum += exp(-hQ/H);
			if (hQ <= 0) {
				cpSum = 1e+20;
				// cpSum = 0.0;
				break;
			}
		}
		//Need to scale by d_cp here instead of outside the loop because d_cp varies with the length of CP
		float odCP = cpSum * d_cp;
		T += exp(-b * (abSum + odCP + odP)); // Moved "* d_ab;" outside the loop for performance
	}
	// return TurboColormap(T.x);
	vec3 I = lI * b * gamma(dot(normalize(B-A), normalize(l-pl.xyz))) * (T * d_ab);
	return I;
}

void main() {
	float timescale = 1.0/2.0;
	vec3 light_pos = vec3(50.0 * sin(time*timescale), 50.0, 50.0 * cos(time*timescale));

	vec2 uv = 2.0 * gl_FragCoord.xy / resolution - 1.0;
	uv.x *= resolution.x / resolution.y;

	vec3 ro = eye_pos;
	vec3 rd = normalize(dir_mat * vec3(uv, -focal_length));
	// vec3 rd = normalize(fposition.xyz - eye_pos);
	vec4 planet_atm = planet;
	planet_atm.w += atmosphere_height;

	// vec3 rd = normalize(fposition.xyz - eye_pos);
	//View ray intersection with planet surface sphere
	vec3 si = sphereIntersection(ro, rd, planet);
	//View ray intersection with atmosphere sphere
	vec3 sia = sphereIntersection(ro, rd, planet_atm);
	//Nearest point of intersection on atmosphere (should change this for camera within the atmosphere case)
	vec3 pa = ro + rd*sia.y;

	vec3 color = vec3(0.0);
	float b = sia.z;
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
	color += atmosphere(pa, ro + rd*b, planet, atmosphere_height, light_pos, vec3(1.0), planet_scale_factor, samples);

	// float d = ((sia.z - sia.y) - (si.z - si.y));
	// float unscientificScatter = dot(pa - planet.xyz, normalize(light_pos - planet.xyz))*0.5 + 0.5;
	// unscientificScatter *= unscientificScatter;
	// color += vec3(0.5, 0.72, 0.99) * smoothstep(0.965, 1.0, dot(rd, normalize(planet.xyz-eye_pos))) * d * unscientificScatter;
	// if (sia.x < 0.0)
	// 	color = vec3(0.1);

	// float gamma = 2.2;
	// color = color / (color + vec3(1.0)); //Tone mapping.
	// color = pow(color, vec3(1.0 / gamma)); //Gamma correction.
	float fExposure = p_and_p;
	color = 1.0 - exp(-fExposure * color);
	LFragment = vec4(color, 1.0);
}