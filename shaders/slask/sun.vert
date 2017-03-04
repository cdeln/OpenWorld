#version 150

in vec3 inPosition;
in vec2 inTexCoord;

out vec2 varTexCoord;
out float sunAngle;

uniform mat4 projMatrix;
uniform mat4 mdlMatrix;
uniform vec3 sunDir;

void main(void)
{
    vec3 posDir = normalize(inPosition);
    sunAngle = acos(dot(posDir, normalize(sunDir)));
    varTexCoord = inTexCoord;
	gl_Position = projMatrix * vec4(mat3(mdlMatrix) * inPosition, 1);
}
