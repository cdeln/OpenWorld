#version 150

in vec3 inPosition;
in vec3 inNormal;

out vec3 varNormal;
out vec3 varDirection;

uniform mat4 projMatrix;
uniform mat4 mdlMatrix;
uniform vec3 position;
uniform vec3 direction;

void main(void)
{
    mat3 normalMatrix = mat3(mdlMatrix);
    varNormal = normalMatrix * inNormal;
    varDirection = normalMatrix * direction;
	gl_Position = projMatrix * mdlMatrix * vec4(inPosition + position, 1.0);
}
