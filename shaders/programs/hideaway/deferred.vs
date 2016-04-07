#version 330 

in vec3 vPos; 
in vec3 vColor; 
in vec3 vNormal; 

//uniform mat4 NMVM;
uniform mat4 model_view_matrix;
uniform mat4 projection_matrix;
uniform mat4 model_matrix;
uniform mat4 transp_model_matrix;

out vec3 fPos;
out vec3 fObjectPos;
out vec3 fColor;
out vec3 fNormal;

void main()
{
	//vec4 vertex_view_pos =  MVM * vec4(vPos, 1);
	//gl_Position = projection_matrix * vertex_view_pos;
	//Pass along position and normal in camera space.
	//fPos = vec3(vertex_view_pos);
	//fNormal = vec3(vec4(vNormal, 0.0) * NMVM);
	fColor = vColor;
	//Pass along object-space position.
	fObjectPos = vPos;

	gl_Position = projection_matrix * (model_view_matrix * vec4(vPos, 1));
	fPos = vec3(model_matrix * vec4(vPos, 1));
	fNormal = vec3(vec4(vNormal, 0.0) * transp_model_matrix);
}