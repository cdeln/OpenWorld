#version 330

layout(location = 0) in vec3 inPosition;

out float varDist;

uniform mat4 projMatrix;
uniform mat4 mdlMatrix;

void main(void)
{
	gl_Position = projMatrix * mdlMatrix * vec4(inPosition, 1.0);
    varDist = gl_Position.z;
}
