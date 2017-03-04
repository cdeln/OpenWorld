#version 150

in vec3 inPosition;
in vec3 inNormal;

out vec3 varNormal;
out vec3 varLight;
out vec3 varPos;

uniform mat4 projMatrix;
uniform mat4 mdlMatrix;

void main(void)
{
    const vec3 lightSource = normalize(vec3(10,1,0));
	mat3 normalMatrix = mat3(mdlMatrix);
    varNormal = normalize(normalMatrix * inNormal);
    varLight = normalize(normalMatrix * lightSource);

    vec4 worldPos = mdlMatrix * vec4(inPosition, 1.0);
    varPos = vec3(worldPos / worldPos.w);

	gl_Position = projMatrix * worldPos;
}
