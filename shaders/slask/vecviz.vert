#version 150

in vec3 inPosition;
in vec3 inVector;

uniform mat4 mdlMatrix;
uniform mat4 projMatrix;

out Vertex
{
    vec4 vector;
} vertex;

void main(void)
{
    gl_Position = vec4(inPosition, 1.0);
    vertex.vector = vec4(inVector, 1.0);
}
