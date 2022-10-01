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
vec3 cylinderIntersection(vec3 p, vec3 d, vec3 cp, vec3 cd, float cr);

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
vec3 atmosphere(vec3 ro, vec3 rd, vec4 abcd, float s, vec4 pl, float rA, vec3 l, vec3 lI, float planet_scale_factor, vec2 samples)
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
	float d_ab = distance(abcd.x, abcd.y) / abSamples;

	//Sum of optical depth along AB
	float abSum = 0.0;
	vec3 T = vec3(0.0);

	for (int i = 0; i < abSamples; i++) {
		//Sample point P along the view ray
		vec3 P = ro + rd*mix(abcd[0], abcd[1], (float(i)+0.5)/abSamples);
		//Altitude of P
		float hP = max(distance(pl.xyz, P) - pl.w, 0.0);
		float odP = exp(-hP/H) * d_ab;
		abSum += odP;

		//Find intersections of sunlight ray with atmosphere sphere
		vec3 lro = l;
		vec3 lrd = normalize(P-l);
		vec3 si = sphereIntersection(lro, lrd, vec4(pl.xyz, rA));
		//Point C, where a ray of light travelling towards P from the sun first strikes the atmosphere
		vec3 C = lro + lrd*si.y;
		//Length of a sample segment on CP
		float d_cp = distance(P, C)/cpSamples;
		//Sum of optical depth along CP
		float cpSum = 0.0;
		//PERF(Gavin): Can I find hQ at each sample point in a cheaper way?
		for (int j = 0; j < cpSamples; j++) {
			//Sample point Q along CP
			vec3 Q = mix(P, C, (float(j)+0.5)/cpSamples);
			//Altitude of Q
			float hQ = distance(pl.xyz, Q) - pl.w;

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
	vec3 I = lI * b * gamma(dot(rd, normalize(l-pl.xyz))) * (T * d_ab);
	return I;
}

//Atmosphere intersection nearest camera, other atmosphere intersection, planet, light position, light intensity
//Leaving working one intact, this is where I'm trying hacks to speed it up
//abcd is distance from ro of the start and end of each integration interval
vec3 atmosphere2(vec3 ro, vec3 rd, vec4 abcd, float s, vec4 pl, float rA, vec3 l, vec3 lI, float planet_scale_factor, vec2 samples)
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
	float l_ab = distance(abcd.x, abcd.y);
	float d_ab = l_ab / abSamples;
	//Sum of optical depth along AB
	float abSum = 0.0;
	vec3 T = vec3(0.0);
	//Find altitude of first atmosphere intersection
	// float hi1 = distance(A, pl.xyz) - pl.w;
	// float m = sqrt(rA*rA-s*s);
	//q(t) depends on s, the half-distance of the ray-atmosphere chord

	for (int i = 0; i < abSamples; i++) {
		//Sample point P along the view ray
		float p = mix(abcd[0], abcd[1], (float(i)+0.5)/abSamples);
		//Altitude of p
		float np = p - abcd[0];
		float hP = sqrt((s-np)*(s-np)+rA*rA-s*s);
		float odP = exp(-hP/H) * d_ab;
		abSum += odP;

		//Still need upper-P until I work out the stuff below in terms of lower-p
		vec3 P = ro + rd*p;

		//Find intersections of sunlight direction with atmosphere sphere
		vec3 lro = l;
		vec3 lrd = normalize(P-l);
		vec3 si = sphereIntersection(lro, lrd, vec4(pl.xyz, rA));
		//Point C, where a ray of light travelling towards P from the sun first strikes the atmosphere
		vec3 C = lro + lrd*si.y;
		//Length of a sample segment on CP
		// float np = p/l_ab;
		float l_cp = sqrt(rA*rA - np*np + 2*np*s - s*s) - sqrt(rA*rA - s*s);
		float d_cp = l_cp/cpSamples;
		// float d_cp = distance(P, C)/cpSamples;
		//Sum of optical depth along CP
		float cpSum = 0.0;
		//PERF(Gavin): Can I find hQ at each sample point in a cheaper way?
		for (int j = 0; j < cpSamples; j++) {
			//Sample point Q along CP
			vec3 Q = mix(P, C, (float(j)+0.5)/cpSamples);
			//Altitude of Q
			float hQ = distance(pl.xyz, Q) - pl.w;

			cpSum += exp(-hQ/H);
			if (hQ <= 0) {
				cpSum = 1e+20;
				// cpSum = 0.0;
				break;
			}
		}
		/*

		f(x) = sqrt(x^2+y^2+z^2)
		let h(x) = sqrt(x) - R = x^(1-2) - R
		let g(x) = x^2+y^2+z^2
		f'(x) = h'(g(x)) * g'(x)
		      = g(x)^(-1/2) * x
		      = ((x^2+y^2+z^2)^(1/2) - R)^(-1/2) * x
		*/
		//Using the integration of the inner loop from tectonics.js (non-approximation)
		float h = max(distance(pl.xyz, C) - pl.w, 0.0);
		float hPrimeX = pow(h, -0.5) * distance(P, C);
		// cpSum = -(H/hPrimeX) * exp(-h/H);

		//Need to scale by d_cp here instead of outside the loop because d_cp varies with the length of CP
		float odCP = cpSum * d_cp;
		T += exp(-b * (abSum + odCP + odP)); // Moved "* d_ab;" outside the loop for performance
	}
	// return TurboColormap(T.x);
	vec3 I = lI * b * gamma(dot(rd, normalize(l-pl.xyz))) * (T * d_ab);
	return I;
}

//Atmosphere intersection nearest camera, other atmosphere intersection, planet, light position, light intensity
//Leaving working one intact, this is where I'm trying hacks to speed it up
//abcd is distance from ro of the start and end of each integration interval
vec3 atmosphere3(vec3 ro, vec3 rd, vec4 abcd, float s, vec4 pl, float rA, vec3 l, vec3 lI, float planet_scale_factor, vec2 samples)
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
	float l_ab = distance(abcd.x, abcd.y);
	float d_ab = l_ab / abSamples;
	//Sum of optical depth along AB
	float abSum = 0.0;
	vec3 T = vec3(0.0);
	//Find altitude of first atmosphere intersection
	// float hi1 = distance(A, pl.xyz) - pl.w;
	// float m = sqrt(rA*rA-s*s);
	//q(t) depends on s, the half-distance of the ray-atmosphere chord

	// TODO: convert these to use the actual variable names
	float l_ab2 = l_ab*l_ab;
	float l_ad = dot(rd, normalize(l-pl.xyz)) * l_ab;
	float l_ad2 = l_ad*l_ad;
	float l_ae = 2.0*rA*l_ad/l_ab;
	float l_ae2 = l_ae*l_ae;
	float l_ed = sqrt(l_ae2 - l_ad2);
	float l_db = sqrt(l_ab2 - l_ad2);
	float l_eb = l_ed + l_db;
	float l_ad_db = l_ad / l_db;

	for (int i = 0; i < abSamples; i++) {
		//Sample point P along the view ray
		float p = mix(abcd[0], abcd[1], (float(i)+0.5)/abSamples);
		//Altitude of p
		float np = p - abcd[0];
		float hP = sqrt((s-np)*(s-np)+rA*rA-s*s);
		float odP = exp(-hP/H) * d_ab;
		abSum += odP;

		//Still need upper-P until I work out the stuff below in terms of lower-p
		vec3 P = ro + rd*p;

		//Find intersections of sunlight direction with atmosphere sphere
		vec3 lro = l;
		vec3 lrd = normalize(P-l);
		vec3 si = sphereIntersection(lro, lrd, vec4(pl.xyz, rA));
		//Point C, where a ray of light travelling towards P from the sun first strikes the atmosphere
		vec3 C = lro + lrd*si.y;
		//Length of a sample segment on CP
		// float np = p/l_ab;
		//This length is towards an infinite-distance sun, so it looks slighly different.
		// float l_cp = sqrt(rA*rA - np*np + 2.0*np*s - s*s) - sqrt(rA*rA - s*s);

		// TODO: convert these to use the actual variable names
		float t_eb = np * l_db / l_ab + l_ed;
		float q_t_eb = sqrt(t_eb*t_eb - t_eb*l_ae + rA*rA);
		float l_cp = q_t_eb - (l_db - t_eb + l_ed) * l_ad_db;

		// l_cp = distance(P, C);
		float d_cp = l_cp/cpSamples;
		// float d_cp = distance(P, C)/cpSamples;
		//Sum of optical depth along CP
		float cpSum = 0.0;
		//PERF(Gavin): Can I find hQ at each sample point in a cheaper way?

		//Half-length of the chord formed by completing CP
		float sQ0 = (rA*rA+l_cp*l_cp-hP*hP)/(2.0*hP);
		//Squared distance from midpoint of completed-CP-chord to planet center
		float mQ2 = rA*rA - sQ0*sQ0;

		for (int j = 0; j < cpSamples; j++) {
			//Sample point Q along CP
			// vec3 Q = mix(P, C, (float(j)+0.5)/cpSamples);
			//Altitude of Q
			// float hQ = distance(pl.xyz, Q);

			//Alpha along CP, 0.0 at C, 1.0 at P (approximately, based on sample count)
			float a_cp = (float(j)+0.5)/cpSamples;
			//Right triangle: length of adjacent, opposite, hypotenuse are aQ, sqrt(mQ2), hQ
			float aQ = sQ0 - a_cp;
			float hQ = sqrt(aQ*aQ + mQ2);
			//Convert hQ to altitude
			hQ -= pl.w;

			cpSum += exp(-hQ/H);
			if (hQ <= 0.0) {
				cpSum = 1e+20;
				// cpSum = 0.0;
				break;
			}
		}
		/*

		f(x) = sqrt(x^2+y^2+z^2)
		let h(x) = sqrt(x) - R = x^(1-2) - R
		let g(x) = x^2+y^2+z^2
		f'(x) = h'(g(x)) * g'(x)
		      = g(x)^(-1/2) * x
		      = ((x^2+y^2+z^2)^(1/2) - R)^(-1/2) * x
		*/
		//Using the integration of the inner loop from tectonics.js (non-approximation)
		float h = max(distance(pl.xyz, C) - pl.w, 0.0);
		float hPrimeX = pow(h, -0.5) * distance(P, C);
		// cpSum = -(H/hPrimeX) * exp(-h/H);

		//Need to scale by d_cp here instead of outside the loop because d_cp varies with the length of CP
		float odCP = cpSum * d_cp;
		T += exp(-b * (abSum + odCP + odP)); // Moved "* d_ab;" outside the loop for performance
	}
	// return TurboColormap(T.x);
	vec3 I = lI * b * gamma(dot(rd, normalize(l-pl.xyz))) * (T * d_ab);
	return I;
}

void main() {
	float timescale = 0.0;
	float light_dist = 50.0;
	vec3 light_pos = vec3(light_dist * sin(time*timescale), light_dist, light_dist * cos(time*timescale));

	vec2 uv = 2.0 * gl_FragCoord.xy / resolution - 1.0;
	uv.x *= resolution.x / resolution.y;

	vec3 ro = eye_pos;
	vec3 rd = normalize(dir_mat * vec3(uv, -focal_length));
	// vec3 rd = normalize(fposition.xyz - eye_pos);
	vec4 planet_atm = planet;
	planet_atm.w += atmosphere_height;

	//View ray intersection with planet surface sphere
	vec3 si = sphereIntersection(ro, rd, planet);
	//View ray intersection with atmosphere sphere
	vec3 sia = sphereIntersection(ro, rd, planet_atm);
	float b = sia.z;

	//Plane normal, cuts the planet into sun-facing and non-sun-facing
	vec3 pn = normalize(light_pos - planet.xyz);
	vec3 ci = cylinderIntersection(ro, rd, planet.xyz, pn, planet.w);
	//I need to consider six points:
	//0. Ray starts in atmosphere (same as 1)
	//1. Ray enters atmosphere
	//2. Ray enters shadow
	//3. Ray hits planet
	//4. Ray exits shadow
	//5. Ray exits atmosphere
	//These are used differently depending on the case.
	//IS = In-Scattering, OS = Out-Scattering
	//Viewed from space:
	//A. Ray enters atmosphere, exits atmosphere. Integrate IS/OS from 1 to 5.
		//1, 5,5,5
	//B. Ray enters atmosphere, hits planet (above shadow plane). Integrate IS/OS from 1 to 3.
		//1, 3,3,3
	//C. Ray enters atmosphere, hits shadow, then planet (below shadow plane). Integrate IS/OS from 1 to 2.
		//1, 2,2,2
	//D. Ray enters atmosphere, hits shadow, exits atmosphere, exits shadow. Integrate IS/OS from 1 to 2 (similar to C).
		//1, 2, 5,5
	//E. Ray enters atmosphere, hits shadow, exits shadow, exits atmosphere. Integrate IS/OS from 1 to 2, OS from 2 to 4, IS/OS from 4 to 5.
		//1, 2, 4, 5
	//F. Ray enters shadow, hits planet. No integration needed.
		//1,1,1,1 or 2,2,2,2 or 3,3,3,3
	//G. Ray enters shadow, enters atmosphere, exits shadow, then exits atmosphere. Integrate OS from 1 to 4, IS/OS from 4 to 5.
		//1,1, 4, 5
	//H. Ray enters shadow, exits shadow. No integration needed.
		//2,2,2,2
	//I. Ray enters shadow, enters atmosphere, exits atmosphere, exits shadow. No integration needed.
		//1,1,1,1
	//Viewed from within atmosphere:
	//J. Ray starts in sunlight, exits atmosphere. Integrate from 0 to 5.
		//0,0,0, 5
	//K. Ray starts in sunlight, enters shadow, exits atmosphere. Integrate from 0 to 2.
		//0, 2,2,2
	//L. Ray starts in sunlight, enters shadow, exits shadow, exits atmosphere. Integrate IS/OS from 0 to 2, OS from 2 to 4, IS/OS from 4 to 5.
		//0, 2, 4, 5
	//M. Ray starts in shadow, exits atmosphere. No integration needed.
		//0,0,0,0
	//N. Ray starts in shadow, exits shadow, exits atmosphere. Integrate OS from 0 to 4, IS/OS from 4 to 5.
		//0,0, 4, 5

	//This could be encoded as distances along the view ray rd.
	//A vec4 could hold up to three intervals, starting nearest the camera and moving away.
	//encode the IS/OS, OS, IS/OS intervals by putting their start and end points in the vector,
	//then integrate 3 times unconditionally. If an interval's end is the same as its start, the interval is 0-length and should be ignored.
	//(probably will end up being multiplied by 0 and will not affect the result)
	//x_y is IS/OS, y_z is OS, z_w is IS/OS. Any can be 0-length.

	//Random guess (think through the cases, see if it would work)
	//x. max(0, dist to first atmosphere intersection)
	//y. max(x, dist to first shadow intersection)
	//z. max(y, dist to second shadow intersection)
	//w. max(z, dist to second atmosphere intersection)
	//Works with: A(1115), 

	// vec4 intervals = vec4();

	// dot(a, b) where b is a unit vector is the scalar projection of a onto b.
	// If I want to have a plane centered at the planet origin to cap the shadow cylinder,
	// I can determine if a point lies above or below that plane by subtracting the planet center from it
	// and then dotting that vector with the plane normal.
	// This will give the point's distance (positive or negative) from that plane.
	// if (ci.x != -1.0) {
	// 	vec3 ca = ro+rd*ci.x;
	// 	float above = dot(pn, ca - planet.xyz);
	// 	if (above < 0.0)
	// 		b = max(ci.x, sia.y);
	// }

	vec3 color = vec3(0.0);
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
	// float planet_scale_factor = 6371000.0 / (1.0 + 0.02);
	// float H = 8500.0 / (6371000.0 / (1.0 + 0.02));
	//float H = scale_height / planet_scale_factor;
	//scale height is 8500.0
	//planet_scale is 6371000.0
	//planet_radius is 1
	//atmosphere height is 0.02
	if (uv.x+uv.y > 0.0) {
		color += atmosphere(ro, rd, vec4(sia.y, b, 0.0, 0.0), 0.0, planet, planet_atm.w, light_pos, vec3(1.0), planet_scale_factor, samples);
		// color += vec3(.2,.0,.0);
	} else {
		color += atmosphere3(ro, rd, vec4(sia.y, b, 0.0, 0.0), (sia.z-sia.y)/2.0, planet, planet_atm.w, light_pos, vec3(1.0), planet_scale_factor, samples);
	}

	// float d = ((sia.z - sia.y) - (si.z - si.y));
	// float unscientificScatter = dot(pa - planet.xyz, normalize(light_pos - planet.xyz))*0.5 + 0.5;
	// unscientificScatter *= unscientificScatter;
	// color += vec3(0.5, 0.72, 0.99) * smoothstep(0.965, 1.0, dot(rd, normalize(planet.xyz-eye_pos))) * d * unscientificScatter;
	// if (sia.x < 0.0)
	// 	color = vec3(0.1);

	// float gamma = 2.2;
	// color = color / (color + vec3(1.0)); //Tone mapping.
	// color = pow(color, vec3(1.0 / gamma)); //Gamma correction.
	// float fExposure = p_and_p;
	float fExposure = 1.0;
	color = 1.0 - exp(-fExposure * color);
	LFragment = vec4(color, 1.0);
}

/*
Speedup / quality-improvement idea:

Find a way to exploit the symmetry of the inner loop to find a simple function for it.
The inner loop essentially sums up optical depth from some point until it exits the atmosphere,
following a path pointing directly towards the sun (at infinity for simplicity).
If I can guarantee that it will never hit the planet (that is, never run the inner loop from within the planet's shadow)
then this should be a function of two variables, ex.:
- initial altitude and "dot(sun-planet, point-planet)"
- x and y in a cross-section of the atmosphere

Some parametrizations may make the math easier.
If this function is slow/unwieldy, I can attempt to find an approximation to it.
This can be done with a Taylor polynomial of the partial derivatives of the function with respect to its two variables.
The Taylor polynomial should be easy it integrate, allowing me to collapse the outer loop as well.

2021-10-17
The inner loop has an analytical integral, but the second integral doesn't appear to be tractable.
It's possible I could compute it efficiently using summed-area-tables (integral images).
If I precompute a matrix where y represents the altitude of the midpoint of a ray passing through the atmosphere,
and x represents the distance along that ray, each cell can be the sum of the optical depth up to that point.
Computing the sum for any segment of any ray can be done with two samples of the table and a subtraction.

2022-1-08
I watched a YouTube video about Padé approximations, they may be a good fit for the inner loop. If I can find a good one,
it may be tractable to integrate it to get a good 2D closed-form approximation.

2022-2-02
My current goal is to figure out how to use LAPACK (through Apple's Accelerate framework)
to solve the system of linear equations I need to find the Padé approximant coefficients. If it is possible to keep the
solution in terms of the derivatives of the function I'm approximating, I can have a function of 2+ variables by solving
for the cooefficients at runtime.

2022-2-05
I think finding the Padé coefficients (solving a system of linear equations) requires the Taylor series coefficients to
not be in terms of other variables, so that approach may not work. However, the function I am trying to approximate is
already one function divided by another. What if I simply find Taylor series approximations to the numerator and denominator,
producing a ratio of two polynomials? Depending on where I truncate the series, can I make the order of the numerator and 
denominator polynomials the same? I can then split this up into easily-integrable chunks using partial fraction decomposition.

Should I teach the computer how to do the chain rule to save myself some busywork on computing the Taylor series to N terms?
Essentially I'd just need a way to take an expression and recursively produce an expression for its derivative.

2022-4-24
I've simplified some of the atmospheric integration to be 2D, I'll continue until I have done so completely.
Next I think I will figure out how to sort the distances for atmosphere / shadow cylinder intersections so I can make the
function handle intervals with only out-scattering correctly. I'm also considering making a 3D graph of the surface of the
function for optical depth (parametrized by s and t) so I can tweak values of a polynomial/Padé approximation manually
(by superimposing both surfaces so I can try to match them).

*/
