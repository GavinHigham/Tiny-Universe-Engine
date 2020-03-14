#version 330 core
layout (triangles_adjacency) in;
layout (triangle_strip, max_vertices = 18) out;

uniform vec3 gLightPos;
uniform int zpass;
uniform float log_depth_intermediate_factor;

in vec3 gPos[6];
in vec3 gNormal[6];
uniform mat4 projection_view_matrix;

float EPSILON = 0.0001;

// Emit a quad using a triangle strip
//void EmitQuadLines(vec3 StartVertex, vec3 EndVertex, vec3 lvStart, vec3 lvEnd, vec3 lpStart, vec3 lpEnd)
void EmitQuadLines(vec3 lvStart, vec3 lvEnd, vec3 lpStart, vec3 lpEnd)
{
	gl_Position = projection_view_matrix * vec4(lpStart, 1.0);
	gl_Position.z = (log2(max(1e-6, 1.0 + gl_Position.z)) * log_depth_intermediate_factor - 1.0) * gl_Position.w;
	EmitVertex();
	gl_Position = projection_view_matrix * vec4(lpEnd, 1.0);
	gl_Position.z = (log2(max(1e-6, 1.0 + gl_Position.z)) * log_depth_intermediate_factor - 1.0) * gl_Position.w;
	EmitVertex();
	gl_Position = projection_view_matrix * vec4(lvStart, 0.0);
	gl_Position.z = (log2(max(1e-6, 1.0 + gl_Position.z)) * log_depth_intermediate_factor - 1.0) * gl_Position.w;
	EmitVertex();
	gl_Position = projection_view_matrix * vec4(lvEnd , 0.0);
	gl_Position.z = (log2(max(1e-6, 1.0 + gl_Position.z)) * log_depth_intermediate_factor - 1.0) * gl_Position.w;
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
	vec3 normal = cross(e1, e2); //The face normal.
	vec3 lv0 = gLightPos - gPos[0]; //From vertex 0 to the light
	vec3 lv2 = gLightPos - gPos[2]; //From vertex 2 to the light
	vec3 lv4 = gLightPos - gPos[4]; //From vertex 4 to the light
	vec3 lp0 = (gPos[0] - lv0 * EPSILON);
	vec3 lp2 = (gPos[2] - lv2 * EPSILON);
	vec3 lp4 = (gPos[4] - lv4 * EPSILON);
	if (dot(normal, lv0) > 0) {
		if (zpass == 0) {
			//Front cap
			gl_Position = projection_view_matrix * vec4(lp0, 1.0);
			gl_Position.z = (log2(max(1e-6, 1.0 + gl_Position.z)) * log_depth_intermediate_factor - 1.0) * gl_Position.w;
			EmitVertex();
			gl_Position = projection_view_matrix * vec4(lp4, 1.0);
			gl_Position.z = (log2(max(1e-6, 1.0 + gl_Position.z)) * log_depth_intermediate_factor - 1.0) * gl_Position.w;
			EmitVertex();
			gl_Position = projection_view_matrix * vec4(lp2, 1.0);
			gl_Position.z = (log2(max(1e-6, 1.0 + gl_Position.z)) * log_depth_intermediate_factor - 1.0) * gl_Position.w;
			EmitVertex();
			EndPrimitive();

			//Back cap
			gl_Position = projection_view_matrix * vec4(-lv0, 0.0);
			gl_Position.z = (log2(max(1e-6, 1.0 + gl_Position.z)) * log_depth_intermediate_factor - 1.0) * gl_Position.w;
			EmitVertex();
			gl_Position = projection_view_matrix * vec4(-lv2, 0.0);
			gl_Position.z = (log2(max(1e-6, 1.0 + gl_Position.z)) * log_depth_intermediate_factor - 1.0) * gl_Position.w;
			EmitVertex();
			gl_Position = projection_view_matrix * vec4(-lv4, 0.0);
			gl_Position.z = (log2(max(1e-6, 1.0 + gl_Position.z)) * log_depth_intermediate_factor - 1.0) * gl_Position.w;
			EmitVertex();
			EndPrimitive();
		}

		normal = cross(e3,e1);

		if (dot(normal, lv0) <= 0)
			EmitQuadLines(-lv0, -lv2, lp0, lp2);

		normal = cross(e4,e5);

		if (dot(normal, lv2) <= 0)
			EmitQuadLines(-lv2, -lv4, lp2, lp4);

		normal = cross(e2,e6);

		if (dot(normal, lv4) <= 0)
			EmitQuadLines(-lv4, -lv0, lp4, lp0);
	}
}