#version 330 core
layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

in vec3 gPos[3];
in vec3 gColor[3];
in vec3 gNormal[3];
out vec3 fPos;
out vec3 fColor;
out vec3 fNormal;

void main() {
	for (int i = 0; i < 3; i++) {
	    gl_Position = gl_in[i].gl_Position;
	    fPos = gPos[i];
	    fColor = gColor[i];
	    fNormal = gNormal[i];
	    EmitVertex();
	}
    
    EndPrimitive();
} 