-- vertex.GL33 --

layout(location = 1) in vec2 pos;

out vec2 position;

void main()
{
	position = pos;
	gl_Position = vec4(pos, -0.5, 1);
}

-- fragment.GL33 --

uniform vec2 iResolution;
uniform vec4 iMouse;
uniform vec2 iTime;
uniform float iFocalLength;
uniform float brightness = 1.0;
uniform int samples = 10;
uniform float render_dist = 100.0;
uniform vec3 spiral_origin = vec3(0.0);
uniform vec3 eye_pos = vec3(0.0);
uniform vec4 tweaks = vec4(1.0);
uniform vec4 tweaks2 = vec4(1.0);
uniform mat3 dir_mat;
uniform float rotation = 500.0;
uniform vec4 bulge = vec4(50.0, 10.0, 1.0, 1.0);

in vec2 position; 
out vec4 LFragment;

const float PHI = 1.61803398874989; //Golden ratio
const float PI = 3.1415926535;
const float INVPI = 1.0/PI;
const float galaxy_diameter = 80.0;
const vec3 bulge_color = vec3(0.992, 0.941, 0.549);

#define NOISE

bool in_galaxy(vec3 p)
{
	float l = length(p.xz);
	float d = l*l / bulge.w;
	return l < galaxy_diameter && (abs(p.y) < bulge.x/(d + 1.0) || length(p)/bulge.z < 2.0);
}

float galaxy_density(vec3 p, float bulge_density)
{
	//This line is responsible for a lot of the visual interest of the spiral,
	//and is probably the first thing that should be tweaked to improve the appearance.
	p += (normalize(p) * (snoise(p / tweaks.x) - 0.4)) * tweaks.y; //Domain transformation.
	float r = rotation / 3000.0 * length(p.xz) + atan(p.x, p.z);   //Sort-of polar coordinates
	float d2 = dot(p.xz, p.xz) / bulge.w;
	return tweaks.z *
		max(pow(0.5 * sin(2.0 * r) + 0.5, tweaks2.y), bulge_density) * //Spiral and bulge
		(1.0 - smoothstep(0, bulge.x/(1.0 + d2), abs(p.y)))          * //Galaxy profile
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
	vec3 ray_start = render_dist * dir_mat * dir + eye_pos;
	// vec3 ray_start = render_dist * normalize(dir);
	vec3 step = (eye_pos - ray_start) / samples;
	float step_dist = length(step);
	float dist = 0.0;
	vec3 color = vec3(0.0);

#ifdef NOISE
	// vec3 noise_step = step * snoise(ray_start * 10.0 * iTime.x);
	vec3 noise_step = step * fract(snoise(ray_start * 10.0) + PHI * iTime.y);
	// vec3 noise_step = step * fract(int(gl_FragCoord.x + gl_FragCoord.y)/2.0 + PHI * iTime.y);
	ray_start += noise_step;
	dist += length(noise_step);

#else
	float golden_step = mod(PHI * iTime.y * step_dist, step_dist) / step_dist;
	ray_start += golden_step * step;
	dist += golden_step;
#endif
	// float by2 = bulge.y * bulge.y;

	vec3 p = ray_start;
	for (int i = 0; i < samples && dist < render_dist; i++, dist += step_dist, p += step) {

		// float r = d/2.0;
		vec3 p2 = p - spiral_origin;
		if (in_galaxy(p2)) {
			float lp2 = length(p2);

			float bulge_density = pow(max(2.0 - lp2/bulge.z, 0.0), tweaks2.z);
			float density = galaxy_density(p2, bulge_density);
			float eps = tweaks.w;
			float dif = clamp(
				(
					galaxy_density(p2 + eps*normalize(-p2), bulge_density) -
					density
				) /
				eps, 
				0.0, 1.0);

			float clamped_density = clamp(density, 0.0, 1.0);
			// float clamped_bulge_density = clamp(bulge_density, 0.0, 1.0);
			float bulge_light_intensity = bulge.z*bulge.z * 2.0 * PI / (10.0 * lp2); //Roughly solid angle of bulge from p2's vantage point.
			color =
				(
					color + //Color from previous steps
					mix(color_ramp(clamped_density * step_dist * 30.0), bulge_color * bulge_density, bulge_density) + //Emissivity
					bulge_light_intensity * tweaks2.x * dif*bulge_color //Diffuse lighting
				) *
				(vec3(1.0) - vec3(0.8, 0.9, 0.99)*clamped_density); //Attenuation
		}
	}
	color /= samples;

	return color * brightness;
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