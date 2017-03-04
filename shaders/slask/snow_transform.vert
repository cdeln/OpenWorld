#version 430

in vec3 inPosition;
out vec3 outPosition;

void main(void)
{
    outPosition = vec3(inPosition.x,inPosition.y-0.1,inPosition.z);
	//gl_Position = projMatrix * mdlMatrix * vec4(inPosition, 1.0);
}
