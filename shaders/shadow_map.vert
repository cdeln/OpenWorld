#version 330

in vec3 inPosition;

uniform mat4 MVP;

void main(void)
{
	gl_Position = MVP * vec4(inPosition, 1.0);
}
