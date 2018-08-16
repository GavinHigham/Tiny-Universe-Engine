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
uniform mat3 dir_mat;
uniform float rotation = 500.0;

in vec2 position; 
out vec4 LFragment;

const float phi = 1.61803398874989; //Golden ratio

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
	return clamp(pow(dot(normalize(p - spiral_origin), normalize(rp)), 16.0), 0, 1);

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

vec3 color_ramp(float v)
{

	return vec3(
		smoothstep(70, 100, v),
		smoothstep(20, 100, v),
		smoothstep(0, 100, v));
}

void main() {
	vec3 dir;
	//Render color ramp to the top
	if (gl_FragCoord.y > iResolution.y - 20) {
		LFragment = vec4(color_ramp(1000*gl_FragCoord.x/iResolution.x), 1.0);
	} else {
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
		vec3 noise_step = step * fract(snoise(ray_start * 10.0) + phi * iTime.y);
		ray_start += noise_step;
		dist += length(noise_step);

#else
		float golden_step = mod(phi * iTime.y * step_dist, step_dist) / step_dist;
		ray_start += golden_step * step;
		dist += golden_step;
#endif

		for (int i = 0; i < samples && dist < render_dist; i++, dist += step_dist) {
			vec3 p = ray_start + step * i;
			float d = distance(p, spiral_origin);
			// float r = d/2.0;
			float r = 1/d * rotation;// - iTime;
			float c = cos(r), s = sin(r);
			float density = spiral(p, d, c, s);
			sum += density;
			color = (color + color_ramp(density * step_dist * 30.0)) * ((vec3(1.0) - vec3(0.8, 0.9, 0.99)*density));
		}
		sum *= step_dist/float(samples);
		color /= float(samples);

		LFragment = vec4(
			color * brightness,
			// color * iMouse.x + (dir_matrix[0] + dir_matrix[1] + dir_matrix[2])*1000,
			//color_ramp( sum * iMouse.x ),
		1.0);
	}
}

-- deferred.GL33 --
uniform vec2 iResolution;
uniform sampler2D accum_buffer;
uniform int num_frames_accum = 1;

in vec2 position; 
out vec4 LFragment;

void main() {
	vec2 tx_coord = gl_FragCoord.xy / iResolution;
	LFragment = vec4(texture(accum_buffer, tx_coord).xyz / float(num_frames_accum), 1.0);
}