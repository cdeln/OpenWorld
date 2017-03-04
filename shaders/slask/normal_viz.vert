#version 150

in vec3 inPosition;
in vec3 inNormal;

uniform mat4 mdlMatrix;
uniform mat4 projMatrix;

out Vertex
{
    vec4 normal;
} vertex;

void main(void)
{
    gl_Position = vec4(inPosition, 1.0);
    vertex.normal = vec4(inNormal, 1.0);
}
