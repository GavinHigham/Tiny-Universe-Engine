#version 140
uniform vec3 light_pos;
uniform float emit;
uniform float alpha;
out vec4 LFragment;
in vec3 camPos;
in vec3 fPos;
in vec4 fColor;
in vec3 fNormal;

void main() {
	float dens = 40;
	float scal = 3/4;
	float intensity = 150;
	float distance = distance(fPos, light_pos);
	float diffuse = intensity * clamp(dot(normalize(light_pos-fPos), fNormal), 0, 1) / (distance*distance);

	vec3 h = normalize(normalize(camPos-fPos) + normalize(light_pos-fPos));
	float specular = pow(max(0.0, dot(h, normalize(fNormal))), 32.0);
	float ambient = 0.1;
	LFragment = vec4(vec3(fColor) * ((1.0-ambient)*diffuse + ambient + emit) + specular, alpha);
	//LFragment = vec4(fNormal, fColor.a);
}