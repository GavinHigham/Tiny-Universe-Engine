#version 330 core
layout (triangles_adjacency) in;
layout (triangle_strip, max_vertices = 3) out;

in vec3 gPos[6];
in vec3 gColor[6];
in vec3 gNormal[6];
out vec3 fPos;
out vec3 fColor;
out vec3 fNormal;

void main() {
	for (int i = 1; i < 6; i+=2) {
	    gl_Position = gl_in[i].gl_Position;
	    fPos = gPos[i];
	    fColor = gColor[i];
	    fNormal = gNormal[i];
	    EmitVertex();
	}
    
    EndPrimitive();
} 