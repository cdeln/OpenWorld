#version 150

in vec3 inPosition;
in vec3 inNormal;

out vec3 varNormal;
out vec3 varLight;

uniform mat4 projMatrix;
uniform mat4 mdlMatrix;

void main(void)
{
    const vec3 light = normalize(vec3(10,1,0));

    mat3 normalMatrix = mat3(mdlMatrix);
    varNormal = normalMatrix * inNormal;
    varLight = normalMatrix * light;
	gl_Position = projMatrix * mdlMatrix * vec4(inPosition, 1.0);
}
