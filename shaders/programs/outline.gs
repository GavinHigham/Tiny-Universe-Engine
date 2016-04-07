#version 330 core
layout (triangles_adjacency) in;
layout (line_strip, max_vertices = 6) out;

uniform vec3 uOrigin;

in vec3 gPos[6];

void EmitSegment(int StartIndex, int EndIndex)
{
    gl_Position = gl_in[StartIndex].gl_Position;
    EmitVertex();
    gl_Position = gl_in[EndIndex].gl_Position;
    EmitVertex();
    EndPrimitive();
}

void main() {
    vec3 e1 = gPos[2] - gPos[0];
    vec3 e2 = gPos[4] - gPos[0];
    vec3 e3 = gPos[1] - gPos[0];
    vec3 e4 = gPos[3] - gPos[2];
    vec3 e5 = gPos[4] - gPos[2];
    vec3 e6 = gPos[5] - gPos[0];
    vec3 normal = cross(e1, e2);
    vec3 LightDir = uOrigin - gPos[0];
        if (dot(normal, LightDir) > 0.00001) {

        normal = cross(e3,e1);

        if (dot(normal, LightDir) <= 0) {
            EmitSegment(0, 2);
        }

        normal = cross(e4,e5);
        LightDir = uOrigin - gPos[2];

        if (dot(normal, LightDir) <=0) {
            EmitSegment(2, 4);
        }

        normal = cross(e2,e6);
        LightDir = uOrigin - gPos[4];

        if (dot(normal, LightDir) <= 0) {
            EmitSegment(4, 0);
        }
    }
}