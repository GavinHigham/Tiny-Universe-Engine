-- vertex.GL33 --

layout(location = 1) in vec2 pos;

out vec2 position;

void main()
{
	position = pos;
	gl_Position = vec4(pos, -0.5, 1.0);
}

-- fragment.GL33 --

uniform vec2 iResolution;
uniform vec3 iTime;
uniform float iFocalLength;
uniform mat3 dir_mat;
uniform vec3 eye_pos = vec3(0.0);
uniform float brightness = 1.0;
uniform float rotation = 500.0;
uniform float galaxy_diameter = 80.0;
uniform vec4 tweaks1 = vec4(1.0);
uniform vec4 tweaks2 = vec4(1.0);
uniform vec4 bulge = vec4(50.0, 10.0, 1.0, 1.0);
uniform vec4 absorption = vec4(0.2, 0.1, 0.01, 0.0);
uniform int samples = 10;
uniform float render_dist = 100.0;
vec3 spiral_origin = vec3(0.0);
uniform float freshness = 0.1;
vec4 transmittance = vec4(1.0) - absorption;

in vec2 position; 
out vec4 LFragment;

const float PHI = 1.61803398874989; //Golden ratio
const float PI = 3.1415926535;
const float INVPI = 1.0/PI;
const float E = 2.7182818284;
const vec3 bulge_color = vec3(0.992, 0.941, 0.549);

// #define EAT_NOISE
// #define BACK_TO_FRONT //Controls if rays go camera->max-depth or max-depth->camera
#define NOISE //Controls if rays are jittered relative to neighbor pixels
#define BOUNDING_SPHERE //Controls if rays start and end on the galaxy's bounding sphere

//Somewhat-fast check if a sample point is within the galaxy
bool in_galaxy(vec3 p)
{
	float l = length(p.xz);
	float d = l*l / bulge.w;
	return l < galaxy_diameter && (abs(p.y) < tweaks2.w/(d + 1.0) || length(p)/bulge.y < 2.0);
}

float snoise(vec3 v);
vec3 sphereIntersection(vec3 p, vec3 d, vec4 s);

//Returns noise at a position, with a certain number of octaves
float snoise_oct(vec3 v, int oct)
{
	float sum = 0.0;
	float q = 1.0;
	for (int i = 0; i < 10 && i < oct; i++) {
		float s = abs(snoise(v*q));
		q *= 2;
		sum += s / q;
	}
	return sum;
}

//Density of the spiral portion of the galaxy, no noise or profile, etc.
float spiral_density(vec3 p)
{
	float r = atan(p.x, p.z) + rotation / 3000.0 * length(p.xz);
	// float r = atan(p.x*pow(E, 1.0/length(p.xz)), 0.7*p.z);
	return pow(
		sin(2.0 * r) * 0.5 + 0.5, //Smoothly wrap to 0.0-1.0. Could possibly replace with fract?
		tweaks1.x);
}

//Density of the galaxy, accounting for spiral, noise, galaxy profile, disk radius.
float galaxy_density(vec3 p, float bulge_density, float noise_scale, float noise_strength)
{
	float s = 1.0;
#ifdef EAT_NOISE
	s *= clamp(snoise_oct(p / (noise_scale), 3) * noise_strength, 0.0, 1.0);
#else
	//Distort the sample point by noise (domain transformation)
	//This line is responsible for a lot of the visual interest of the spiral,
	//and is probably the first thing that should be tweaked to improve the appearance.
	p += (normalize(p) * (snoise_oct(p / noise_scale, 3) - 0.4)) * noise_strength; //Domain transformation.
#endif
	s *= spiral_density(p);
	float d2 = dot(p.xz, p.xz) / bulge.w;
	return tweaks1.w *
		max(s, bulge_density) * //Spiral and bulge
		(1.0 - smoothstep(0, tweaks2.w/(1.0 + d2), abs(p.y))) * //Galaxy profile
		(1.0 - smoothstep(0, galaxy_diameter, 2*length(p.xz))); //Fade spiral out into disk shape
}

//Unscientific mapping of (I think, been a while since I wrote this) density to a color.
vec3 color_ramp(float v)
{
	// return vec3(smoothstep(0, 100, 1.0));
	return vec3(
		smoothstep(70, 100, v),
		smoothstep(20, 100, v),
		smoothstep(0, 100, v));
}

vec3 raymarch(vec3 ray_start, vec3 step, float max_dist)
{
	float step_dist = length(step);
	float dist = 0.0;
	vec3 color = vec3(0.0);
	vec3 sdt = vec3(1.0);
	float od = 0.0;

	vec3 p = ray_start;
	{
		for (int i = 0; i < samples && dist < max_dist; i++, dist += step_dist, p += step) {

			// float r = d/2.0;
			vec3 p2 = p - spiral_origin;
			if (in_galaxy(p2)) {
				float lp2 = length(p2);

				float bulge_density = pow(max(2.0 - lp2/bulge.y, 0.0), bulge.z);
				float density = galaxy_density(p2, bulge_density, tweaks1.y, tweaks1.z);
				float clamped_density = clamp(density, 0.0, 1.0);
				float eps = tweaks2.y;
				float dif = clamp(
					(galaxy_density(p2 - eps*normalize(p2), bulge_density, tweaks1.y, tweaks1.z) - density) / eps,
					0.0, 1.0);

				od += density * step_dist;
				// sdt *= transmittance.xyz*(1 - clamped_density);
				// float clamped_bulge_density = clamp(bulge_density, 0.0, 1.0);
				float bulge_light_intensity = bulge.y*bulge.y * 2.0 * PI / (10.0 * lp2); //Roughly solid angle of bulge from p2's vantage point.

#ifdef BACK_TO_FRONT
				color =
					(
						color + //Color from previous steps
						mix(color_ramp(clamped_density * step_dist * 30.0), bulge_color * bulge_density, bulge_density) * tweaks2.z + //Emissivity
						bulge_light_intensity * tweaks2.x * dif*bulge_color //Diffuse lighting
					) *
					(vec3(1.0) - transmittance.xyz*clamped_density); //Attenuation
#else
				if (length(sdt) < 0.005) {
					// samples = i;
					// color = vec3(1.0, 0.0, 0.0);
					break;
				}
				color +=
					(
						(color_ramp(clamped_density * step_dist * 30.0) + bulge_color * bulge_density) * tweaks2.z + //Emissivity
						bulge_light_intensity * tweaks2.x * dif*bulge_color //Diffuse lighting
					) /*  * exp(-od * absorption.xyz) */; //Attenuation
#endif
			}
		}
	}
	//More correct to do this at each step, but this is way faster.
	color *= exp(-od * absorption.xyz) / samples;
	return clamp(color * brightness, 0.0, 100000.0);
	// return vec3(si);
}

void main() {
	vec2 uv = 2.0 * gl_FragCoord.xy / iResolution - 1.0;
	uv.x *= iResolution.x / iResolution.y;

	vec3 rd = normalize(dir_mat * vec3(uv, -iFocalLength));
	vec3 ro = eye_pos;
	float start_bias = 0;
	float max_dist = render_dist;

	#ifdef BOUNDING_SPHERE
		vec3 si = sphereIntersection(eye_pos, rd, vec4(spiral_origin, galaxy_diameter/2.0));
		start_bias += max(0.0, si.y); //Second intersection with the sphere
		max_dist = min(si.z, render_dist);
	#endif

	float step_dist = render_dist / samples;
	vec3 step = rd * step_dist;

	#ifdef BACK_TO_FRONT
		ro = (max_dist * rd) + ro;
		step = -step;
	#endif

	#ifdef NOISE
		// vec3 noise_step = step * snoise(ro * 10.0 * iTime.x);
		// start_bias += step_dist * fract(snoise((ro + max_dist*rd) * 10.0) + PHI * iTime.y);
		start_bias += step_dist * fract(snoise((ro + max_dist*rd) * 10.0) + iTime.z);
		// vec3 noise_step = step * fract(int(gl_FragCoord.x + gl_FragCoord.y)/2.0 + PHI * iTime.y);
	#else
		float golden_step = mod(PHI * iTime.y * step_dist, step_dist) / step_dist;
		start_bias += golden_step * step_dist;
	#endif

	// if (gl_FragCoord.y < 10)
	// 	LFragment = vec4(color_ramp(gl_FragCoord.x), 1.0);
	// else
	ro += rd*start_bias;
	LFragment = vec4(raymarch(ro, step, max_dist+start_bias), freshness);
}

-- deferred.GL33 --
uniform vec2 iResolution;
uniform sampler2D accum_buffer;
uniform int num_frames_accum = 1;
float gamma = 2.2;

in vec2 position;
out vec4 LFragment;

void main() {
	vec2 tx_coord = gl_FragCoord.xy / iResolution;
	//Temporal denoising
	vec3 color = texture(accum_buffer, tx_coord).xyz / float(num_frames_accum);
	//Tone mapping
	// color = color / (color + vec3(1.0));
	//Gamma correction
	// color = pow(color, vec3(1.0 / gamma));
	LFragment = vec4(color, 1.0);
}

-- deferred.cubemap.GL33 --

uniform samplerCube accum_cube;
uniform int num_frames_accum = 1;
float gamma = 2.2;

in vec2 position;
out vec4 LFragment;

void main() {
	//Temporal denoising
	vec3 color = vec3(0.0);

	vec2 p = mod(position + 1.0, 0.5) * 4.0 - 1.0;
	// Cases for each 1/16th of the screen
	if (position.y < -0.5) {
		if (position.x < -0.5)
			;
		else if (position.x < 0.0)
			color = texture(accum_cube, normalize(vec3(p.x, -1.0, -p.y))).xyz; //-y
		else if (position.x < 0.5)
			;
		else
			;
	} else if (position.y < 0.0) {
		if (position.x < -0.5)
			color = texture(accum_cube, normalize(vec3(1.0, -p.yx))).xyz; //+x
		else if (position.x < 0.0)
			color = texture(accum_cube, normalize(vec3(-p, -1.0))).xyz; //-z
		else if (position.x < 0.5)
			color = texture(accum_cube, normalize(vec3(-1.0, -p.y, p.x))).xyz; //-x
		else
			color = texture(accum_cube, normalize(vec3(p.x, -p.y, 1.0))).xyz; //z
	} else if (position.y < 0.5) {
		if (position.x < -0.5)
			;
		else if (position.x < 0.0)
			color = texture(accum_cube, normalize(vec3(p.x, 1.0, p.y))).xyz; //+y
		else if (position.x < 0.5)
			;
		else
			;
	} else {
		;
		// if (position.x < -0.5)
		// 	;
		// else if (position.x < 0.0)
		// 	color = texture(accum_cube, normalize(vec3(-1.0, position))).xyz; //-x
		// else if (position.x < 0.5)
		// 	;
		// else
		// 	;
	}
	//Tone mapping
	// color = color / (color + vec3(1.0));
	//Gamma correction
	// color = pow(color, vec3(1.0 / gamma));
	LFragment = vec4(color / float(num_frames_accum), 1.0);
}