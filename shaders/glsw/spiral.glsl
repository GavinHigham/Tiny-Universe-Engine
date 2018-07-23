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
uniform float iTime;
uniform float iFocalLength;
uniform vec3 eye_pos = vec3(0.0);
uniform int samples = 16;
uniform float render_dist = 100.0;
uniform vec3 spiral_origin = vec3(-16, -20, -50);
uniform mat4 view_matrix;

in vec2 position;
out vec4 LFragment;

#define NOISE

vec2 rot(vec2 a, float c, float s)
{
	return vec2(dot(vec2(c, -s), a), dot(vec2(s, c), a));
}

float spiral(vec3 p, float d, float c, float s)
{
	//Domain transformation about y-axis of spiral_center
	vec3 rp = p;
	rp.xz = rot(p.xz - spiral_origin.xz, c, s) + spiral_origin.xz;
	float v = smoothstep(10.0, 0.0, sqrt(1+rp.x*rp.x));
	// if (v > 0.01) {
	// 	v *= snoise(p/4.0);
	// }
	return v;
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
		//dir = (view_matrix * vec4(dir, 1)).xyz;

		//Stepping towards the camera.
		vec3 ray_start = render_dist * normalize(dir);
		vec3 step = (eye_pos - ray_start) / samples;
#ifdef NOISE
		ray_start += step * snoise(ray_start * 10.0);
#endif
		float step_dist = length(step);
		float dist = 0.0;
		float sum = 0.0;
		vec3 color = vec3(0.0);

		for (int i = 0; i < samples && dist < render_dist; i++, dist += step_dist) {
			vec3 p = ray_start + step * i;
			float d = distance(p, spiral_origin);
			float r = iMouse.y/2000.0*d + iTime;
			float c = cos(r), s = sin(r);
			float density = spiral(p, d, c, s);
			sum += density;
			color = (color + color_ramp(density * step_dist * 30.0)) * ((vec3(1.0) - vec3(0.8, 0.9, 0.99)*density));
		}
		sum *= step_dist/float(samples);
		color /= float(samples);

		LFragment = vec4(
			color * iMouse.x,
			//color_ramp( sum * iMouse.x ),
		1.0);
	}
}