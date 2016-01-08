#version 330

in vec3 fPos;
in vec3 fObjectPos;
in vec3 fColor;
in vec3 fNormal;

layout (location = 0) out vec3 WorldPosOut; 
layout (location = 1) out vec3 DiffuseOut; 
layout (location = 2) out vec3 NormalOut;

void main() 
{ 
	WorldPosOut = fPos;
	vec3 noise_coords = fObjectPos*4;
	float noise = 0;
	// int num_iterations = 5;
	// for (int i = 0; i < num_iterations; i++) {
	// 	noise = noise + snoise(noise_coords);
	// 	noise_coords *= 2;
	// }
	// noise = (noise + 2)/(3 + num_iterations);
	DiffuseOut = fColor + noise;
	NormalOut = normalize(fNormal); 
}