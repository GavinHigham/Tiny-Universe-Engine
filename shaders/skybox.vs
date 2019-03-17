#version 330 

in vec3 vPos;

uniform float log_depth_intermediate_factor;

uniform mat4 model_matrix;
uniform mat4 model_view_projection_matrix;
out vec3 fPos;
// out vec3 fColor;

// vec4 positions[] = vec4[](vec4(-1.0, -1.0, -0.5, 1.0),
//                           vec4(1.0, -1.0, -0.5, 1.0),
//                           vec4(-1.0, 1.0, -0.5, 1.0));

void main()
{
	fPos = vec3(model_matrix * vec4(vPos, 1));
	gl_Position = model_view_projection_matrix * vec4(vPos, 1);
	// fColor = abs(gl_Position.xyz);
	gl_Position = gl_Position.xyww;
	// if (gl_VertexID < 3)
	// 	gl_Position = positions[gl_VertexID % 3];
	// gl_Position.z = 0.0;
	// gl_Position.z = log2(max(1e-6, 1.0 + gl_Position.w)) * log_depth_intermediate_factor - 1.0; //I'm not sure why, but -2.0 works better for me.
}