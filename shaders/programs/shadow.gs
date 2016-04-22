#version 330 core
layout (triangles_adjacency) in;
layout (line_strip, max_vertices = 14) out;

uniform vec3 uOrigin;
uniform vec3 gLightPos;

in vec3 gPos[6];
uniform mat4 model_view_matrix;
uniform mat4 projection_matrix;

float EPSILON = 0.0001;

void EmitSegment(int StartIndex, int EndIndex)
{
	gl_Position = gl_in[StartIndex].gl_Position;
	EmitVertex();
	gl_Position = gl_in[EndIndex].gl_Position;
	EmitVertex();
	EndPrimitive();
}

// Emit a quad using a triangle strip
void EmitQuadLines(vec3 StartVertex, vec3 EndVertex)
{
    // Vertex #1: the starting vertex (just a tiny bit below the original edge)
    vec3 LightDir = normalize(StartVertex - gLightPos); 
    gl_Position = projection_matrix * (model_view_matrix * vec4((StartVertex + LightDir * EPSILON), 1.0));
    EmitVertex();

    // Vertex #2: the starting vertex projected to infinity
    gl_Position = projection_matrix * (model_view_matrix * vec4(LightDir, 0.0));
    EmitVertex();

    EndPrimitive();

    // Vertex #3: the ending vertex (just a tiny bit below the original edge)
    LightDir = normalize(EndVertex - gLightPos);
    gl_Position = projection_matrix * (model_view_matrix * vec4((EndVertex + LightDir * EPSILON), 1.0));
    EmitVertex();

    // Vertex #4: the ending vertex projected to infinity
    gl_Position = projection_matrix * (model_view_matrix * vec4(LightDir , 0.0));
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
	vec3 LightDir = uOrigin - gLightPos;
	LightDir = uOrigin - gPos[0];
	if (dot(normal, LightDir) > 0) {

		normal = cross(e3,e1);

		if (dot(normal, LightDir) <= 0) {
			vec3 StartVertex = gPos[0];
            vec3 EndVertex = gPos[2];
            EmitQuadLines(StartVertex, EndVertex);
			//EmitSegment(0, 2);
		}

		normal = cross(e4,e5);
		LightDir = uOrigin - gPos[2];

		if (dot(normal, LightDir) <=0) {
			vec3 StartVertex = gPos[2];
            vec3 EndVertex = gPos[4];
            EmitQuadLines(StartVertex, EndVertex);
			//EmitSegment(2, 4);
		}

		normal = cross(e2,e6);
		LightDir = uOrigin - gPos[4];

		if (dot(normal, LightDir) <= 0) {
			vec3 StartVertex = gPos[4];
            vec3 EndVertex = gPos[0];
            EmitQuadLines(StartVertex, EndVertex);
			//EmitSegment(4, 0);
		}
	}
	// gl_Position = vec4(-1, -1, 0.4, 1);
	// EmitVertex();
	// gl_Position = vec4(1, 1, 0.4, 1);
	// EmitVertex();
	// EndPrimitive();
}