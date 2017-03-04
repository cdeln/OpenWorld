#version 150

in vec3 inPosition;
in vec3 inNormal;
in vec2 inTexCoord;

out vec2 varTexCoord;
out float varLight;

uniform mat4 projMatrix;
uniform mat4 mdlMatrix;
uniform vec3 sunDir;

void main(void)
{
    varLight = clamp(dot(normalize(inNormal),normalize(sunDir)),0,1);
    varTexCoord = inTexCoord;
	gl_Position = projMatrix * mdlMatrix * vec4(inPosition,1.0);
}
