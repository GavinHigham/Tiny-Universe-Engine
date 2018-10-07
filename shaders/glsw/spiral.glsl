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
uniform int samples = 5;
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

#define NOISE

vec2 rot(vec2 a, float c, float s)
{
	return vec2(dot(vec2(c, -s), a), dot(vec2(s, c), a));
}

float spiral(vec3 p, float d, float c, float s)
{
	//Domain transformation about y-axis of spiral_origin
	vec3 rp = p - spiral_origin;
	rp.xz = rot(rp.xz, c, s);
	// rp.y = 0;
	// return clamp(pow(dot(normalize(p - spiral_origin), normalize(rp)), 16.0), 0, 1);
	return clamp(pow(normalize(rp - spiral_origin).x, 16.0), 0, 1);

	// rp.xz = rot(p.xz, c, s);
	// rp.xz = rot(p.xz - spiral_origin.xz, c, s);// + spiral_origin.xz;
	// rp.y = 0.0;
	// return 1 - dot(rp, p);
	// float y = rp.y - spiral_origin.y;
	// float y = rp.y;
	// float x = rp.z/sqrt(10+rp.x*rp.x/16);
	// float v = smoothstep(5.0, 0.5, x*x + y*y);
	// if (v > 0.01) {
	// 	v *= snoise(p/4.0);
	// }
	//return v;
}

float galaxy_density(vec3 p, float bulge_density)
{
	p += (normalize(p) * (snoise(p / tweaks.x) - 0.4)) * tweaks.y;
	float d = length(p);
	float r = rotation / 3000.0 * length(p.xz) + 
		atan(p.x, p.z); //Angle of sample point about the spiral origin, modulo a
	// float c = cos(r), s = sin(r);
	// float density = spiral(p, d, c, s);
	// float density = mix(fract(r*INVPI), sin(r), 0.5*sin(iTime.x)+0.5) * clamp(5.0, -5.0, p.y);
	float d2 = d / bulge.y;
	return tweaks.z * max((0.5 * sin(2.0 * r) + 0.5), bulge_density) * //Spiral and bulge
		     (1.0 - smoothstep(0, bulge.x/(1.0 + d2*d2), abs(p.y))) *
	                 	//(1.0 - smoothstep(-15.0, 15.0, abs(p.y))) * //Cap spiral on top and bottom
		              (1.0 - smoothstep(    0,   80, length(p.xz)));  //Fade spiral out into disk shape
}

vec3 color_ramp(float v)
{

	return vec3(
		smoothstep(70, 100, v),
		smoothstep(20, 100, v),
		smoothstep(0, 100, v));
}

vec3 bulge_color(float bulge_density)
{
	return vec3(0.992, 0.941, 0.549) * bulge_density;
}

float dither17(vec2 pos, float FrameIndexMod4)
{
    return fract(dot(vec3(pos.xy, FrameIndexMod4), ivec3(2, 7, 23) / 17.0));
}

vec3 raymarch()
{
	vec3 dir;
	//Render color ramp to the top
	// if (gl_FragCoord.y > iResolution.y - 20) {
	// 	return color_ramp(1000*gl_FragCoord.x/iResolution.x);
	// } else 
	{
		dir.xy = 2.0 * gl_FragCoord.xy / iResolution - 1.0;
		dir.z = -iFocalLength;

		//Stepping towards the camera.
		// vec3 ray_start = render_dist * normalize(dir_mat * dir) + eye_pos;
		vec3 ray_start = render_dist * dir_mat * dir + eye_pos;
		// vec3 ray_start = render_dist * normalize(dir);
		vec3 step = (eye_pos - ray_start) / samples;
		float step_dist = length(step);
		float dist = 0.0;
		float sum = 0.0;
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

		for (int i = 0; i < samples && dist < render_dist; i++, dist += step_dist) {
			vec3 p = ray_start + step * i;

			// float r = d/2.0;
			vec3 p2 = p - spiral_origin;
			float lp2 = length(p2);

			float bulge_density = max(2.0 - lp2/bulge.z, 0.0);
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
			sum += clamped_density;
			float bulge_light_intensity = bulge.z*bulge.z * 2.0 * PI / (10.0 * lp2);
			color =
				(
					color + //Color from previous steps
					mix(color_ramp(clamped_density * step_dist * 30.0), bulge_color(bulge_density), bulge_density) + //Emissivity
					bulge_light_intensity * tweaks2.x * dif*bulge_color(1.0) //Diffuse lighting
				) *
				(vec3(1.0) - vec3(0.8, 0.9, 0.99)*clamped_density); //Attenuation
		}
		sum *= step_dist/float(samples);
		color /= float(samples);

		return color * brightness;
	}
}

vec2 c = vec2(0.0,0.0);
void main() {
	vec2 uv = 2.0 * gl_FragCoord.xy / iResolution - 1.0;
	uv.x *= iResolution.x / iResolution.y;

	// float r = rotation * distance(uv, c) + atan(uv.x, uv.y);
	// LFragment = vec4(vec3(mix(fract(r*INVPI), sin(r), 0.5*sin(iTime.x)+0.5)), 1.0);
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