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
uniform vec4 iMouse;
uniform vec2 iTime;
uniform float iFocalLength;
uniform mat3 dir_mat;
uniform vec3 eye_pos = vec3(0.0);
uniform float brightness = 1.0;
uniform float rotation = 500.0;
uniform vec4 tweaks = vec4(1.0);
uniform vec4 tweaks2 = vec4(1.0);
uniform vec4 bulge = vec4(50.0, 10.0, 1.0, 1.0);
uniform vec4 absorption = vec4(0.2, 0.1, 0.01, 0.0);
uniform int samples = 10;
uniform float render_dist = 100.0;
uniform vec3 spiral_origin = vec3(0.0);
vec4 transmittance = vec4(1.0) - absorption;

in vec2 position; 
out vec4 LFragment;

const float PHI = 1.61803398874989; //Golden ratio
const float PI = 3.1415926535;
const float INVPI = 1.0/PI;
const float galaxy_diameter = 80.0;
const vec3 bulge_color = vec3(0.992, 0.941, 0.549);

#define EAT_NOISE
#define NOISE
#define FRONT_TO_BACK

bool in_galaxy(vec3 p)
{
	float l = length(p.xz);
	float d = l*l / bulge.w;
	return l < galaxy_diameter && (abs(p.y) < bulge.x/(d + 1.0) || length(p)/bulge.z < 2.0);
}

float snoise_oct(vec3 v, int oct)
{
	float sum = 0.0;
	float q = 1.0;
	for (int i = 0; i < 10 && i < oct; i++) {
		float s = snoise(v*q);
		q *= 2;
		sum += s / q;
	}
	return sum;
}

// //Returns discriminant in x, lambda1 in y, lambda2 in z.
vec3 sphereIntersection(vec3 p, vec3 d, vec4 s)
{
	//Solve the quadratic equation to find a lambda if there are intersections.
	float b = -dot(d, (p-s.xyz));
	float dd = dot(d,d);

	float discriminant = dd*s.w*s.w;
	if (discriminant >= 0) {
		float sqd = sqrt(discriminant);
		//If discriminant == 0, there's only one intersection, and lambda1 == lambda2.
		//This should be fairly uncommon.
		float lambda1 = (b+sqd)/dd;
		float lambda2 = (b-sqd)/dd;
		return vec3(discriminant, lambda1, lambda2);
	}
	//No intersection.
	return vec3(discriminant, 0, 0);
}

// //First three values are the closest point of intersection if there is one.
// //The last value is 0 if there is no intersection.
// vec3 sphereIntersection(vec3 p, vec3 endRay, vec4 s)
// {
// 	vec3 d = normalize(p-endRay);
// 	//Solve the quadratic equation to find a lambda if there are intersections.
// 	float a = pow(d.x, 2) + pow(d.y, 2) + pow(d.z, 2);
// 	float b = 2*d.x*(p.x-s.x) + 2*d.y*(p.y-s.y) + 2*d.z*(p.z-s.z);
// 	float c = pow(p.x-s.x, 2) + pow(p.y-s.y, 2) + pow(p.z-s.z, 2) - pow(s.w, 2);
// 	float discriminant = pow(b, 2) - 4*a*c;
// 	if (discriminant == 0) {//Uncommon case, one real root.
// 		float lambda = (-b)/(2*a);
// 		// return lambda;
// 		return vec3(discriminant, lambda, 0);
// 	}
// 	if (discriminant > 0) {
// 		float lambda1 = (-b+sqrt(discriminant))/(2*a);
// 		float lambda2 = (-b-sqrt(discriminant))/(2*a);
// 		// float lambda = min(abs(lambda1), abs(lambda2));
// 		// return lambda;
// 		return vec3(discriminant, lambda1, lambda2);
// 		//Compute the point of intersection.
// 	}
// 	//Otherwise no intersection.
// 	return vec3(discriminant, 0, 0);
// 	// return DRAW_DISTANCE + 1; //Far enough not to be drawn.
// }

float spiral_density(vec3 p)
{
	float r = atan(p.x, p.z) + //Polar coordinates
		rotation / 3000.0 * length(p.xz);
	return pow(
		sin(2.0 * r) * 0.5 + 0.5, //Smoothly wrap to 0.0-1.0
		tweaks2.y);
}

float galaxy_density(vec3 p, float bulge_density, float noise_scale, float noise_strength)
{
	float s = spiral_density(p);
#ifdef EAT_NOISE
	s *= clamp(snoise_oct(p / (noise_scale), 3) * noise_strength, 0.0, 1.0);
#else
	//This line is responsible for a lot of the visual interest of the spiral,
	//and is probably the first thing that should be tweaked to improve the appearance.
	p += (normalize(p) * (snoise(p / noise_scale) - 0.4)) * noise_strength; //Domain transformation.
#endif
	float d2 = dot(p.xz, p.xz) / bulge.w;
	return tweaks.z *
		max(s, bulge_density) * //Spiral and bulge
		(1.0 - smoothstep(0, bulge.x/(1.0 + d2), abs(p.y))) * //Galaxy profile
		(1.0 - smoothstep(0, galaxy_diameter, length(p.xz)));          //Fade spiral out into disk shape
}


vec3 color_ramp(float v)
{
	return vec3(
		smoothstep(70, 100, v),
		smoothstep(20, 100, v),
		smoothstep(0, 100, v));
}

vec3 raymarch()
{
	vec3 dir;
	dir.xy = 2.0 * gl_FragCoord.xy / iResolution - 1.0;
	dir.z = -iFocalLength;

	//Stepping towards the camera.
	// vec3 ray_start = render_dist * normalize(dir_mat * dir) + eye_pos;
	vec3 ray_end = render_dist * dir_mat * dir + eye_pos;
	// vec3 ray_start = render_dist * normalize(dir);

#ifdef FRONT_TO_BACK
	vec3 step = (ray_end - eye_pos) / samples;
	vec3 ray_start = eye_pos;
#else
	vec3 step = (eye_pos - ray_end) / samples;
	vec3 ray_start = ray_end;
#endif

	float step_dist = length(step);
	float dist = 0.0;
	vec3 color = vec3(0.0);
	vec3 sdt = vec3(1.0);

#ifdef NOISE
	// vec3 noise_step = step * snoise(ray_start * 10.0 * iTime.x);
	vec3 noise_step = step * fract(snoise(ray_end * 10.0) + PHI * iTime.y);
	// vec3 noise_step = step * fract(int(gl_FragCoord.x + gl_FragCoord.y)/2.0 + PHI * iTime.y);
	ray_start += noise_step;
	dist += length(noise_step);

#else
	float golden_step = mod(PHI * iTime.y * step_dist, step_dist) / step_dist;
	ray_start += golden_step * step;
	dist += golden_step;
#endif
	// float by2 = bulge.y * bulge.y;
	vec3 norm_step = normalize(step);

	vec3 si = sphereIntersection(ray_start, norm_step, vec4(spiral_origin, galaxy_diameter/2.0));
	vec3 p = ray_start;// + min(si.y, si.z)*norm_step;
	/*if (si.x <= 0) */{
		for (int i = 0; i < samples && dist < render_dist; i++, dist += step_dist, p += step) {

			// float r = d/2.0;
			vec3 p2 = p - spiral_origin;
			if (in_galaxy(p2)) {
				float lp2 = length(p2);

				float bulge_density = pow(max(2.0 - lp2/bulge.z, 0.0), tweaks2.z);
				float density = galaxy_density(p2, bulge_density, tweaks.x, tweaks.y);
				float clamped_density = clamp(density, 0.0, 1.0);
				float eps = tweaks.w;
				float dif = clamp(
					(galaxy_density(p2 - eps*normalize(p2), bulge_density, tweaks.x, tweaks.y) - density) / eps,
					0.0, 1.0);

				sdt *= transmittance.xyz*(1 - clamped_density);
				// float clamped_bulge_density = clamp(bulge_density, 0.0, 1.0);
				float bulge_light_intensity = bulge.z*bulge.z * 2.0 * PI / (10.0 * lp2); //Roughly solid angle of bulge from p2's vantage point.

#ifdef FRONT_TO_BACK
				if (length(sdt) < 0.05) {
					// samples = i;
					// color = vec3(1.0, 0.0, 0.0);
					break;
				}
				color +=
					(
						mix(color_ramp(clamped_density * step_dist * 30.0), bulge_color * bulge_density, bulge_density) + //Emissivity
						bulge_light_intensity * tweaks2.x * dif*bulge_color //Diffuse lighting
					) * sdt; //Attenuation
#else
				color =
					(
						color + //Color from previous steps
						mix(color_ramp(clamped_density * step_dist * 30.0), bulge_color * bulge_density, bulge_density) * tweaks2.w + //Emissivity
						bulge_light_intensity * tweaks2.x * dif*bulge_color //Diffuse lighting
					) *
					(vec3(1.0) - transmittance.xyz*clamped_density); //Attenuation
#endif
			}
		}
	}
	color /= samples;
	vec3 tint = vec3(0);
	if (si.x < 0)
		tint = vec3(0.25, 0, 0);

	return color * brightness + tint;
}

void main() {
	vec2 uv = 2.0 * gl_FragCoord.xy / iResolution - 1.0;
	uv.x *= iResolution.x / iResolution.y;

	LFragment = vec4(raymarch(), 1.0);
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

	vec2 p = mod( position + 1.0 // 0-2
	                        , 0.5) // 0-0.5 4 times
	                        * 4.0  // 0-2 4 times
	                        - 1.0; // -1-1 4 times
	// Cases for each 1/16th of the screen
	if (position.y < -0.5) {
		if (position.x < -0.5)
			;
		else if (position.x < 0.0)
			color = texture(accum_cube, normalize(vec3(p.x, -1.0, -p.y))).xyz / float(num_frames_accum); //-y
		else if (position.x < 0.5)
			;
		else
			;
	} else if (position.y < 0.0) {
		if (position.x < -0.5)
			color = texture(accum_cube, normalize(vec3(1.0, -p.yx))).xyz / float(num_frames_accum); //+x
		else if (position.x < 0.0)
			color = texture(accum_cube, normalize(vec3(-p, -1.0))).xyz / float(num_frames_accum); //-z
		else if (position.x < 0.5)
			color = texture(accum_cube, normalize(vec3(-1.0, -p.y, p.x))).xyz / float(num_frames_accum); //-x
		else
			color = texture(accum_cube, normalize(vec3(p.x, -p.y, 1.0))).xyz / float(num_frames_accum); //z
	} else if (position.y < 0.5) {
		if (position.x < -0.5)
			;
		else if (position.x < 0.0)
			color = texture(accum_cube, normalize(vec3(p.x, 1.0, p.y))).xyz / float(num_frames_accum); //+y
		else if (position.x < 0.5)
			;
		else
			;
	} else {
		;
		// if (position.x < -0.5)
		// 	;
		// else if (position.x < 0.0)
		// 	color = texture(accum_cube, normalize(vec3(-1.0, position))).xyz / float(num_frames_accum); //-x
		// else if (position.x < 0.5)
		// 	;
		// else
		// 	;
	}
	//Tone mapping
	// color = color / (color + vec3(1.0));
	//Gamma correction
	// color = pow(color, vec3(1.0 / gamma));
	LFragment = vec4(color, 1.0);
}