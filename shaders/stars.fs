#version 330

in vec3 fPos;
out vec4 LFragment;

uniform float stars_radius;

void main() {
	vec2 pd = gl_PointCoord - vec2(0.5);
	float x = sqrt(2*(pd.x*pd.x + pd.y*pd.y));
	//vec3 color = vec3(1-x*1.1, 1-x*1.11, 1-x);
	float b = 1-distance(fPos, vec3(0.0))/stars_radius;
	LFragment = vec4((vec3(pow(b+0.25, 7.5)-0.1, pow(b, 1.8), b+0.3)+vec3(0.1))*(1-x), 1.0);
	//LFragment = vec4(1.0);
	//LFragment = vec4(vec3(attenuation), 1.0);
}