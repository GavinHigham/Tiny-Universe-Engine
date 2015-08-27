#version 140
uniform vec3 sun_light;
out vec4 LFragment;
in vec4 fColor;
in vec3 fNormal;

void main() {
	float diffuse = dot(normalize(sun_light), fNormal);
	float ambient = 0.1;
	LFragment = vec4(vec3(fColor)*((1-ambient)*diffuse + ambient), fColor.a);
	//LFragment = vec4(fNormal, fColor.a);
}