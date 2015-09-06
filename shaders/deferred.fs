#version 330

in vec3 fPos;
in vec3 fColor;
in vec3 fNormal; 

layout (location = 0) out vec3 WorldPosOut; 
layout (location = 1) out vec3 DiffuseOut; 
layout (location = 2) out vec3 NormalOut; 

void main() 
{ 
    WorldPosOut = fPos; 
    DiffuseOut = fColor; 
    NormalOut = normalize(fNormal); 
}