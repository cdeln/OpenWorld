#version 330

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;

out vec2 varTexCoord;

void main(void)
{
    varTexCoord = inTexCoord;
	gl_Position = vec4(inPosition,1); 
}
