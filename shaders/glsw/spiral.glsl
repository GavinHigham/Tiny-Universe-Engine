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
uniform int samples = 32;
uniform float render_dist = 100.0;
uniform vec3 spiral_origin = vec3(-16, -20, -50);

in vec2 position;
out vec4 LFragment;

vec2 rot(vec2 a, float c, float s)
{
	return vec2(dot(vec2(c, -s), a), dot(vec2(s, c), a));
}

float spiral(vec3 p, float d, float c, float s)
{
	//Domain transformation about y-axis of spiral_center
	vec3 rp = p;
	rp.xz = rot(p.xz - spiral_origin.xz, c, s) + spiral_origin.xz;
	float v = smoothstep(10.0, 0.0, distance(rp, vec3(rp.x, spiral_origin.y, (spiral_origin.z-rp.z)*20.0/min(d, 1.0) + rp.z)));
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
	if (gl_FragCoord.y > iResolution.y - 20) {
		LFragment = vec4(color_ramp(1000*gl_FragCoord.x/iResolution.x), 1.0);
	} else {
		dir.xy = 2.0 * gl_FragCoord.xy / iResolution - 1.0;
		dir.z = -iFocalLength;

		vec3 ray_start = render_dist * normalize(dir);
		//Stepping towards the camera.
		vec3 step = (eye_pos - ray_start) / samples;
		float step_dist = length(step);
		float dist = 0.0;
		float sum = 0.0;

		for (int i = 0; i < samples && dist < render_dist; i++, dist += step_dist) {
			vec3 p = ray_start + step * i;
			float d = distance(p, spiral_origin);
			float r = iMouse.y/2000.0*d + iTime;
			float c = cos(r), s = sin(r);
			sum += spiral(p, d, c, s);
		}

		LFragment = vec4(color_ramp(sum / (iMouse.x / 2000)), 1.0);
	}
}