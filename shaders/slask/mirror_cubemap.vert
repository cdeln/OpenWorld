#version 150

in vec3 inPosition;
in vec3 inNormal;

out vec3 varPosition;
out vec3 varNormal;

uniform mat4 projMatrix;
uniform mat4 mdlMatrix;

void main(void)
{
	gl_Position = projMatrix * mdlMatrix * vec4(inPosition, 1.0);
    varPosition = inPosition;
    varNormal = inNormal;
}
