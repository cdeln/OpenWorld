#version 150

in vec3 inPosition;
in vec3 inNormal;

flat out int instanceID;
flat out int isFaceBack;

uniform mat4 projMatrix;
uniform mat4 mdlMatrix;

void main(void)
{
    instanceID = gl_InstanceID;
    vec3 normal = mat3(mdlMatrix) * inNormal;
    if( normal.z <= 0 )
        isFaceBack = 1;
    else
        isFaceBack = 0;
	gl_Position = projMatrix * mdlMatrix * vec4(inPosition, 1.0);
}
